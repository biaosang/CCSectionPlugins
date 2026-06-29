#include "SectionProfileDialog.h"

#include <ccPointCloud.h>
#include <ccPolyline.h>

#include <QComboBox>
#include <QDialogButtonBox>
#include <QDoubleSpinBox>
#include <QFileDialog>
#include <QFormLayout>
#include <QImage>
#include <QLabel>
#include <QMessageBox>
#include <QPainter>
#include <QPushButton>
#include <QVariant>
#include <QVBoxLayout>

#include <algorithm>
#include <limits>

namespace
{
struct Range
{
	double xMin = 0.0;
	double xMax = 1.0;
	double yMin = 0.0;
	double yMax = 1.0;
};

Range sampleRange(const std::vector<SectionSample>& samples)
{
	Range range;
	if (samples.empty())
	{
		return range;
	}

	range.xMin = range.xMax = samples.front().chainage;
	range.yMin = range.yMax = samples.front().elevation;

	for (const SectionSample& sample : samples)
	{
		range.xMin = std::min(range.xMin, sample.chainage);
		range.xMax = std::max(range.xMax, sample.chainage);
		range.yMin = std::min(range.yMin, sample.elevation);
		range.yMax = std::max(range.yMax, sample.elevation);
	}

	if (range.xMin == range.xMax)
	{
		range.xMax += 1.0;
	}
	if (range.yMin == range.yMax)
	{
		range.yMax += 1.0;
	}

	const double yPadding = (range.yMax - range.yMin) * 0.08;
	range.yMin -= yPadding;
	range.yMax += yPadding;

	return range;
}

QPointF mapToPlot(const SectionSample& sample, const Range& range, const QRect& plotRect)
{
	const double xRatio = (sample.chainage - range.xMin) / (range.xMax - range.xMin);
	const double yRatio = (sample.elevation - range.yMin) / (range.yMax - range.yMin);
	return QPointF(plotRect.left() + xRatio * plotRect.width(),
	               plotRect.bottom() - yRatio * plotRect.height());
}
}

SectionProfilePlot::SectionProfilePlot(QWidget* parent)
	: QWidget(parent)
{
	setMinimumSize(640, 360);
	setAutoFillBackground(true);
}

void SectionProfilePlot::setSamples(const std::vector<SectionSample>& samples)
{
	m_samples = &samples;
	update();
}

QImage SectionProfilePlot::renderImage(const QSize& requestedSize) const
{
	const QSize imageSize = requestedSize.isValid() ? requestedSize : QSize(1600, 900);
	QImage image(imageSize, QImage::Format_ARGB32_Premultiplied);
	image.fill(Qt::white);

	QPainter painter(&image);
	painter.setRenderHint(QPainter::Antialiasing, true);
	paintPlot(painter, image.rect());
	return image;
}

void SectionProfilePlot::paintEvent(QPaintEvent*)
{
	QPainter painter(this);
	painter.setRenderHint(QPainter::Antialiasing, true);
	painter.fillRect(rect(), Qt::white);
	paintPlot(painter, rect());
}

void SectionProfilePlot::paintPlot(QPainter& painter, const QRect& bounds) const
{
	const QRect plotRect = bounds.adjusted(72, 24, -28, -56);

	painter.setPen(QPen(QColor(220, 220, 220), 1));
	painter.drawRect(plotRect);

	if (!m_samples || m_samples->empty())
	{
		painter.setPen(QColor(80, 80, 80));
		painter.drawText(plotRect, Qt::AlignCenter, tr("No section samples"));
		return;
	}

	const Range range = sampleRange(*m_samples);

	painter.setPen(QPen(QColor(232, 232, 232), 1));
	for (int i = 1; i < 5; ++i)
	{
		const int x = plotRect.left() + plotRect.width() * i / 5;
		const int y = plotRect.top() + plotRect.height() * i / 5;
		painter.drawLine(x, plotRect.top(), x, plotRect.bottom());
		painter.drawLine(plotRect.left(), y, plotRect.right(), y);
	}

	painter.setPen(QPen(QColor(38, 83, 164), 1));
	QPointF previous;
	bool hasPrevious = false;
	for (const SectionSample& sample : *m_samples)
	{
		const QPointF point = mapToPlot(sample, range, plotRect);
		if (hasPrevious)
		{
			painter.drawLine(previous, point);
		}
		previous = point;
		hasPrevious = true;
	}

	painter.setPen(Qt::NoPen);
	painter.setBrush(QColor(214, 72, 58, 180));
	for (const SectionSample& sample : *m_samples)
	{
		const QPointF point = mapToPlot(sample, range, plotRect);
		painter.drawEllipse(point, 2.0, 2.0);
	}

	painter.setPen(QColor(54, 54, 54));
	painter.drawText(bounds.adjusted(8, 6, -8, -8), Qt::AlignTop | Qt::AlignHCenter, tr("Section profile"));
	painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 24, plotRect.width(), 24),
	                 Qt::AlignCenter,
	                 tr("Chainage"));

	painter.save();
	painter.translate(18, plotRect.center().y());
	painter.rotate(-90);
	painter.drawText(QRect(-plotRect.height() / 2, 0, plotRect.height(), 24),
	                 Qt::AlignCenter,
	                 tr("Elevation"));
	painter.restore();

	painter.drawText(QRect(plotRect.left(), plotRect.bottom() + 4, 120, 18),
	                 Qt::AlignLeft,
	                 QString::number(range.xMin, 'f', 2));
	painter.drawText(QRect(plotRect.right() - 120, plotRect.bottom() + 4, 120, 18),
	                 Qt::AlignRight,
	                 QString::number(range.xMax, 'f', 2));
	painter.drawText(QRect(6, plotRect.top() - 8, 60, 18),
	                 Qt::AlignRight,
	                 QString::number(range.yMax, 'f', 2));
	painter.drawText(QRect(6, plotRect.bottom() - 10, 60, 18),
	                 Qt::AlignRight,
	                 QString::number(range.yMin, 'f', 2));
}

SectionProfileDialog::SectionProfileDialog(const ccPolyline& polyline,
                                           const ccPointCloud& cloud,
                                           QWidget* parent)
	: QDialog(parent)
	, m_polyline(polyline)
	, m_cloud(cloud)
{
	setWindowTitle(tr("Section Profile"));
	resize(820, 560);

	m_halfWidthSpinBox = new QDoubleSpinBox(this);
	m_halfWidthSpinBox->setRange(0.000001, 1000000000.0);
	m_halfWidthSpinBox->setDecimals(6);
	m_halfWidthSpinBox->setValue(0.5);
	m_halfWidthSpinBox->setSuffix(tr(" units"));

	m_elevationComboBox = new QComboBox(this);
	m_elevationComboBox->addItem(tr("X"), 0);
	m_elevationComboBox->addItem(tr("Y"), 1);
	m_elevationComboBox->addItem(tr("Z"), 2);
	m_elevationComboBox->setCurrentIndex(2);

	QFormLayout* formLayout = new QFormLayout;
	formLayout->addRow(tr("Half width"), m_halfWidthSpinBox);
	formLayout->addRow(tr("Elevation axis"), m_elevationComboBox);

	m_plot = new SectionProfilePlot(this);

	m_statusLabel = new QLabel(this);
	m_statusLabel->setTextInteractionFlags(Qt::TextSelectableByMouse);

	QPushButton* recomputeButton = new QPushButton(tr("Recompute"), this);
	m_saveImageButton = new QPushButton(tr("Save image"), this);
	m_exportCsvButton = new QPushButton(tr("Export CSV"), this);

	QDialogButtonBox* buttons = new QDialogButtonBox(Qt::Horizontal, this);
	buttons->addButton(recomputeButton, QDialogButtonBox::ActionRole);
	buttons->addButton(m_saveImageButton, QDialogButtonBox::ActionRole);
	buttons->addButton(m_exportCsvButton, QDialogButtonBox::ActionRole);
	buttons->addButton(QDialogButtonBox::Close);

	QVBoxLayout* mainLayout = new QVBoxLayout(this);
	mainLayout->addLayout(formLayout);
	mainLayout->addWidget(m_plot, 1);
	mainLayout->addWidget(m_statusLabel);
	mainLayout->addWidget(buttons);

	connect(recomputeButton, &QPushButton::clicked, this, &SectionProfileDialog::recompute);
	connect(m_saveImageButton, &QPushButton::clicked, this, &SectionProfileDialog::saveImage);
	connect(m_exportCsvButton, &QPushButton::clicked, this, &SectionProfileDialog::exportCsv);
	connect(buttons, &QDialogButtonBox::rejected, this, &QDialog::reject);
	connect(m_halfWidthSpinBox, qOverload<double>(&QDoubleSpinBox::valueChanged), this, &SectionProfileDialog::recompute);
	connect(m_elevationComboBox, qOverload<int>(&QComboBox::currentIndexChanged), this, &SectionProfileDialog::recompute);

	recompute();
}

void SectionProfileDialog::recompute()
{
	SectionExtractionSettings settings;
	settings.halfWidth = m_halfWidthSpinBox->value();
	settings.elevationDimension = m_elevationComboBox->currentData().toUInt();

	QString errorMessage;
	if (!SectionProfileExtractor::extract(m_polyline, m_cloud, settings, m_samples, errorMessage))
	{
		m_samples.clear();
		m_plot->setSamples(m_samples);
		updateStatus(errorMessage);
		m_saveImageButton->setEnabled(false);
		m_exportCsvButton->setEnabled(false);
		return;
	}

	m_plot->setSamples(m_samples);
	updateStatus(tr("%1 point(s) extracted. Polyline length: %2")
	             .arg(m_samples.size())
	             .arg(SectionProfileExtractor::polylineLength(m_polyline), 0, 'f', 3));
	m_saveImageButton->setEnabled(true);
	m_exportCsvButton->setEnabled(true);
}

void SectionProfileDialog::saveImage()
{
	const QString filename = QFileDialog::getSaveFileName(this,
	                                                     tr("Save section profile image"),
	                                                     QString(),
	                                                     tr("PNG image (*.png);;JPEG image (*.jpg *.jpeg)"));
	if (filename.isEmpty())
	{
		return;
	}

	if (!m_plot->renderImage(QSize(1600, 900)).save(filename))
	{
		QMessageBox::warning(this, tr("Save failed"), tr("Could not save the image."));
	}
}

void SectionProfileDialog::exportCsv()
{
	const QString filename = QFileDialog::getSaveFileName(this,
	                                                     tr("Export section CSV"),
	                                                     QString(),
	                                                     tr("CSV file (*.csv)"));
	if (filename.isEmpty())
	{
		return;
	}

	QString errorMessage;
	if (!SectionProfileExtractor::exportCsv(filename, m_samples, errorMessage))
	{
		QMessageBox::warning(this, tr("Export failed"), errorMessage);
	}
}

void SectionProfileDialog::updateStatus(const QString& message)
{
	m_statusLabel->setText(message);
}
