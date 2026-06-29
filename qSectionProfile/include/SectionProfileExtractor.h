#pragma once

#include <CCGeom.h>

#include <QString>
#include <vector>

class ccPointCloud;
class ccPolyline;

struct SectionSample
{
	double chainage = 0.0;
	double elevation = 0.0;
	double offset = 0.0;
	unsigned segmentIndex = 0;
	CCVector3d point;
};

struct SectionExtractionSettings
{
	double halfWidth = 0.5;
	unsigned elevationDimension = 2;
	bool keepLeftRightSign = true;
};

class SectionProfileExtractor
{
public:
	static bool extract(const ccPolyline& polyline,
	                    const ccPointCloud& cloud,
	                    const SectionExtractionSettings& settings,
	                    std::vector<SectionSample>& samples,
	                    QString& errorMessage);

	static bool exportCsv(const QString& filename,
	                      const std::vector<SectionSample>& samples,
	                      QString& errorMessage);

	static double polylineLength(const ccPolyline& polyline);
};
