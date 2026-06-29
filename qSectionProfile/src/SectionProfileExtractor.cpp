#include "SectionProfileExtractor.h"

#include <ccPointCloud.h>
#include <ccPolyline.h>

#include <QFile>
#include <QTextStream>

#include <algorithm>
#include <cmath>
#include <limits>

namespace
{
double component(const CCVector3d& point, unsigned dimension)
{
	return dimension == 0 ? point.x : (dimension == 1 ? point.y : point.z);
}

double planarDot(const CCVector3d& a, const CCVector3d& b, unsigned elevationDimension)
{
	double result = 0.0;
	for (unsigned dimension = 0; dimension < 3; ++dimension)
	{
		if (dimension != elevationDimension)
		{
			result += component(a, dimension) * component(b, dimension);
		}
	}
	return result;
}

double planarNorm2(const CCVector3d& point, unsigned elevationDimension)
{
	return planarDot(point, point, elevationDimension);
}

CCVector3d toDouble(const CCVector3* point)
{
	return CCVector3d(point->x, point->y, point->z);
}

double planarCrossSign(const CCVector3d& a, const CCVector3d& b, unsigned elevationDimension)
{
	if (elevationDimension == 0)
	{
		return a.y * b.z - a.z * b.y;
	}
	if (elevationDimension == 1)
	{
		return a.z * b.x - a.x * b.z;
	}
	return a.x * b.y - a.y * b.x;
}
}

double SectionProfileExtractor::polylineLength(const ccPolyline& polyline)
{
	double length = 0.0;
	const unsigned count = polyline.size();
	for (unsigned i = 1; i < count; ++i)
	{
		const CCVector3d a = toDouble(polyline.getPoint(i - 1));
		const CCVector3d b = toDouble(polyline.getPoint(i));
		length += (b - a).norm();
	}
	return length;
}

bool SectionProfileExtractor::extract(const ccPolyline& polyline,
                                      const ccPointCloud& cloud,
                                      const SectionExtractionSettings& settings,
                                      std::vector<SectionSample>& samples,
                                      QString& errorMessage)
{
	samples.clear();

	if (polyline.size() < 2)
	{
		errorMessage = QStringLiteral("The selected polyline needs at least two vertices.");
		return false;
	}

	if (cloud.size() == 0)
	{
		errorMessage = QStringLiteral("The selected cloud is empty.");
		return false;
	}

	if (settings.halfWidth <= 0.0)
	{
		errorMessage = QStringLiteral("The section half width must be greater than zero.");
		return false;
	}

	struct Segment
	{
		CCVector3d a;
		CCVector3d b;
		CCVector3d ab;
		double length2 = 0.0;
		double startChainage = 0.0;
	};

	std::vector<Segment> segments;
	segments.reserve(polyline.size() - 1);

	double runningLength = 0.0;
	for (unsigned i = 1; i < polyline.size(); ++i)
	{
		Segment segment;
		segment.a = toDouble(polyline.getPoint(i - 1));
		segment.b = toDouble(polyline.getPoint(i));
		segment.ab = segment.b - segment.a;
		segment.length2 = planarNorm2(segment.ab, settings.elevationDimension);
		segment.startChainage = runningLength;

		const double length = std::sqrt(segment.length2);
		runningLength += length;

		if (segment.length2 > std::numeric_limits<double>::epsilon())
		{
			segments.push_back(segment);
		}
	}

	if (segments.empty())
	{
		errorMessage = QStringLiteral("The selected polyline has no non-zero length segment.");
		return false;
	}

	const double halfWidth2 = settings.halfWidth * settings.halfWidth;
	samples.reserve(cloud.size());

	for (unsigned i = 0; i < cloud.size(); ++i)
	{
		const CCVector3d point = toDouble(cloud.getPoint(i));

		double bestDistance2 = std::numeric_limits<double>::max();
		double bestChainage = 0.0;
		double bestOffset = 0.0;
		unsigned bestSegment = 0;

		for (unsigned s = 0; s < static_cast<unsigned>(segments.size()); ++s)
		{
			const Segment& segment = segments[s];
			const CCVector3d ap = point - segment.a;
			const double t = std::max(0.0, std::min(1.0, planarDot(ap, segment.ab, settings.elevationDimension) / segment.length2));
			const CCVector3d projected = segment.a + segment.ab * t;
			const CCVector3d delta = point - projected;
			const double distance2 = planarNorm2(delta, settings.elevationDimension);

			if (distance2 < bestDistance2)
			{
				bestDistance2 = distance2;
				bestChainage = segment.startChainage + std::sqrt(segment.length2) * t;
				bestOffset = std::sqrt(distance2);

				if (settings.keepLeftRightSign && planarNorm2(segment.ab, settings.elevationDimension) > std::numeric_limits<double>::epsilon())
				{
					bestOffset *= planarCrossSign(segment.ab, delta, settings.elevationDimension) < 0.0 ? -1.0 : 1.0;
				}

				bestSegment = s;
			}
		}

		if (bestDistance2 <= halfWidth2)
		{
			SectionSample sample;
			sample.chainage = bestChainage;
			sample.elevation = component(point, settings.elevationDimension);
			sample.offset = bestOffset;
			sample.segmentIndex = bestSegment;
			sample.point = point;
			samples.push_back(sample);
		}
	}

	std::sort(samples.begin(), samples.end(), [](const SectionSample& lhs, const SectionSample& rhs) {
		if (lhs.chainage == rhs.chainage)
		{
			return lhs.elevation < rhs.elevation;
		}
		return lhs.chainage < rhs.chainage;
	});

	if (samples.empty())
	{
		errorMessage = QStringLiteral("No points were found inside the requested section width.");
		return false;
	}

	errorMessage.clear();
	return true;
}

bool SectionProfileExtractor::exportCsv(const QString& filename,
                                        const std::vector<SectionSample>& samples,
                                        QString& errorMessage)
{
	QFile file(filename);
	if (!file.open(QIODevice::WriteOnly | QIODevice::Text))
	{
		errorMessage = file.errorString();
		return false;
	}

	QTextStream stream(&file);
	stream.setRealNumberNotation(QTextStream::FixedNotation);
	stream.setRealNumberPrecision(6);
	stream << "chainage,elevation,offset,x,y,z,segment_index\n";

	for (const SectionSample& sample : samples)
	{
		stream << sample.chainage << ','
		       << sample.elevation << ','
		       << sample.offset << ','
		       << sample.point.x << ','
		       << sample.point.y << ','
		       << sample.point.z << ','
		       << sample.segmentIndex << '\n';
	}

	return true;
}
