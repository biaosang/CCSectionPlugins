#pragma once

#include "SectionProfileExtractor.h"

#include <QDialog>
#include <QWidget>

#include <vector>

class QComboBox;
class QDoubleSpinBox;
class QLabel;
class QPushButton;
class ccPointCloud;
class ccPolyline;

class SectionProfilePlot : public QWidget
{
	Q_OBJECT

public:
	explicit SectionProfilePlot(QWidget* parent = nullptr);

	void setSamples(const std::vector<SectionSample>& samples);
	QImage renderImage(const QSize& requestedSize) const;

protected:
	void paintEvent(QPaintEvent* event) override;

private:
	void paintPlot(QPainter& painter, const QRect& bounds) const;

	const std::vector<SectionSample>* m_samples = nullptr;
};

class SectionProfileDialog : public QDialog
{
	Q_OBJECT

public:
	SectionProfileDialog(const ccPolyline& polyline,
	                     const ccPointCloud& cloud,
	                     QWidget* parent = nullptr);

private slots:
	void recompute();
	void saveImage();
	void exportCsv();

private:
	void updateStatus(const QString& message);

	const ccPolyline& m_polyline;
	const ccPointCloud& m_cloud;

	QDoubleSpinBox* m_halfWidthSpinBox = nullptr;
	QComboBox* m_elevationComboBox = nullptr;
	QLabel* m_statusLabel = nullptr;
	SectionProfilePlot* m_plot = nullptr;
	QPushButton* m_saveImageButton = nullptr;
	QPushButton* m_exportCsvButton = nullptr;

	std::vector<SectionSample> m_samples;
};
