// ----------------------------------------------------------------------------------------------------
//  Title			FStatisticsView.cpp
//  Description		Implementation of class FStatisticsView
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FStatisticsView.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStatisticsView
// ----------------------------------------------------------------------------------------------------

// Static members -------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStatisticsView::FStatisticsView(QWidget* pParent, Qt::WindowFlags flags)
: QWidget(pParent, flags),
  m_header(8, 6, 180, 0),
  m_rowHeight(18.0f),
  m_rightMargin(6),
  m_listIndex(0),
  m_pLogFile(NULL),
  m_pLogStream(NULL)
{
	m_errorScale.set(0.0, 10.0);
	m_errorInterval = 1.0;

	m_timeScale.set(0.0, 300.0);
	m_timeInterval = 10.0;

	m_statList.resize(HISTORY_SIZE);

	for (int i = 0; i < 7; i++)
		m_timeSum[i] = 0.0;
}

FStatisticsView::~FStatisticsView()
{
	F_SAFE_DELETE(m_pLogStream);
	F_SAFE_DELETE(m_pLogFile);
}

// Public commands ------------------------------------------------------------------------------------

void FStatisticsView::startLog(const QString& logFilePath)
{
	if (m_pLogStream)
		return;

	m_pLogFile = new QFile(logFilePath);
	if (!m_pLogFile->open(QIODevice::WriteOnly))
	{
		F_SAFE_DELETE(m_pLogFile);
		return;
	}

	m_pLogStream = new QTextStream(m_pLogFile);

	QTextStream& stream = *m_pLogStream;
	QString tab("\t");

	// write header
	stream << "FrameID" << tab
		<< "TrackerState" << tab
		<< "NumPoses" << tab << "PoseUsed" << tab
		<< "Previous" << tab << "Prediction" << tab << "Optimization A" << tab << "Optimization B" << tab
		<< "Delta Pos X" << tab << "Delta Pos Y" << tab << "Delta Pos Z" << tab
		<< "Delta Rot X" << tab << "Delta Rot Y" << tab << "Delta Rot Z" << tab << "Delta Lens" << tab
		<< "Var Pos X" << tab << "Var Pos Y" << tab << "Var Pos Z" << tab
		<< "Var Rot X" << tab << "Var Rot Y" << tab << "Var Rot Z" << tab << "Var Lens" << tab
		<< "Var Max" << endl;
}

void FStatisticsView::stopLog()
{
	if (!m_pLogStream)
		return;

	m_pLogStream->flush();
	m_pLogFile->close();

	F_SAFE_DELETE(m_pLogStream);
	F_SAFE_DELETE(m_pLogFile);
}

void FStatisticsView::updateStatistics(FFrameStatistics stats)
{
	if (m_pLogStream)
		_writeLog(stats);

	m_listIndex = (m_listIndex + 1) % HISTORY_SIZE;

	m_timeSum[0] -= m_statList[m_listIndex].tracker.timeSearch * 1000.0;
	m_timeSum[1] -= m_statList[m_listIndex].tracker.timeOptimizationA * 1000.0;
	m_timeSum[2] -= m_statList[m_listIndex].tracker.timeOptimizationB * 1000.0;
	m_timeSum[3] -= m_statList[m_listIndex].detector.timeContourExtraction * 1000.0;
	m_timeSum[4] -= m_statList[m_listIndex].detector.timeContourNormalization * 1000.0;
	m_timeSum[5] -= m_statList[m_listIndex].detector.timeContourAlignment * 1000.0;
	m_timeSum[6] -= m_statList[m_listIndex].detector.timePoseReconstruction * 1000.0;

	m_statList[m_listIndex] = stats;

	m_timeSum[0] += m_statList[m_listIndex].tracker.timeSearch * 1000.0;
	m_timeSum[1] += m_statList[m_listIndex].tracker.timeOptimizationA * 1000.0;
	m_timeSum[2] += m_statList[m_listIndex].tracker.timeOptimizationB * 1000.0;
	m_timeSum[3] += m_statList[m_listIndex].detector.timeContourExtraction * 1000.0;
	m_timeSum[4] += m_statList[m_listIndex].detector.timeContourNormalization * 1000.0;
	m_timeSum[5] += m_statList[m_listIndex].detector.timeContourAlignment * 1000.0;
	m_timeSum[6] += m_statList[m_listIndex].detector.timePoseReconstruction * 1000.0;

	update();
}

// Overrides ------------------------------------------------------------------------------------------

QSize FStatisticsView::sizeHint() const
{
	return QSize(360, 240);
}

void FStatisticsView::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	_paintFramework(painter);
	_paintStatistics(painter);
}

void FStatisticsView::resizeEvent(QResizeEvent* pResizeEvent)
{
	m_header.setHeight(pResizeEvent->size().height() - 2 * m_header.top());
}

// Internal functions ---------------------------------------------------------------------------------

void FStatisticsView::_writeLog(const FFrameStatistics& stats)
{
	F_ASSERT(m_pLogStream);
	QTextStream& stream = *m_pLogStream;
	QString tab("\t");

	// frame number
	stream << stats.frameNumber << tab;

	// tracker state
	stream << ((stats.tracker.state == FLineTrackerState::Tracking) ? 1 : 0) << tab;

	// detector
	stream << stats.detector.numPoses << tab << stats.detector.poseUsed << tab;

	// error per stage
	for (int i = 0; i < FTrackerStatistics::NumStages; i++)
		stream << stats.tracker.errorMean[i] << tab;
	
	// pose delta
	int i0 = m_listIndex;
	int i1 = (m_listIndex + HISTORY_SIZE - 1) % HISTORY_SIZE;
	for (int i = 0; i < 7; i++)
	{
		double pd = m_statList[i0].finalPose[i] - m_statList[i1].finalPose[i];
		stream << pd << tab;
	}

	// variance
	double maxVar = 0.0;
	for (int i = 0; i < 7; i++)
	{
		maxVar = fMax(maxVar, stats.tracker.variance[i]);
		stream << stats.tracker.variance[i] << tab;
	}

	stream << maxVar;

	// endline
	stream << endl;
}

void FStatisticsView::_paintFramework(QPainter& painter)
{
	painter.setPen(Qt::transparent);
	painter.setBrush(QBrush(QColor(72, 72, 72)));
	for (float a = 1.0f; a <= 4.0f; a++)
		painter.drawRect(_fullRow(a));
	for (float a = 6.0f; a <= 7.0f; a++)
		painter.drawRect(_fullRow(a));

	QRect graphRect = _fullRow(9.0f);
	graphRect.setBottom(_rowBottom(15.0f));
	painter.drawRect(graphRect);
	m_graphHeight = graphRect.height();

	painter.setPen(QColor(104, 104, 104));
	int subdiv = (m_errorScale.y() - m_errorScale.x()) / m_errorInterval;
	for (int i = 0; i < subdiv; i++)
	{
		int x = m_header.right() + _xPos(m_errorScale.x() + i * m_errorInterval, m_errorScale);
		painter.drawLine(x, _rowTop(1.0) - 2, x, _rowBottom(4.0) + 2);
	}

	subdiv = (m_timeScale.y() - m_timeScale.x()) / m_timeInterval;
	for (int i = 0; i < subdiv; i++)
	{
		int x = m_header.right() + _xPos(m_timeScale.x() + i * m_timeInterval, m_timeScale);
		painter.drawLine(x, _rowTop(6.0) - 2, x, _rowBottom(7.0) + 2);
	}

	int x0 = m_header.right();
	int x1 = width() - m_rightMargin;
	int y0 = graphRect.top();
	int y1 = y0 + graphRect.height() / 2;
	int y2 = graphRect.bottom();

	painter.drawLine(x0, y0, x1, y0);
	painter.drawLine(x0, y1, x1, y1);
	painter.drawLine(x0, y2, x1, y2);

	painter.drawLine(m_header.left() + 40, y0, m_header.left() + 40, y2);
	painter.drawLine(m_header.left() + 105, y0, m_header.left() + 105, y2);
	painter.drawLine(m_header.right(), y0, m_header.right(), y2);

	painter.setPen(QColor(240, 240, 240));
	painter.drawText(_headerText(0.0), "Error");
	painter.drawText(_contentText(0.0), "Median, Mean, Standard Deviation [Pixels]");
	painter.drawText(_headerText(5.0), "Timing");
	painter.drawText(_headerText(8.0), "Parameter Value          Variance");


	QString timeText = QString("Calculation Time [milliseconds] - Search: %1  |  Opt A: %2  |  Opt B: %3   ---   Extr: %4  |  Norm: %5  |  Fit: %6  |  Pose: %7")
		.arg(m_timeSum[0] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[1] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[2] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[3] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[4] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[5] / HISTORY_SIZE, 0, 'f', 1)
		.arg(m_timeSum[6] / HISTORY_SIZE, 0, 'f', 1);
	painter.drawText(_contentText(5.0), timeText);

	painter.setPen(QColor(200, 200, 200));
	painter.drawText(_headerText(1.0), "Previous Pose");
	painter.drawText(_headerText(2.0), "Prediction");
	painter.drawText(_headerText(3.0), "Optimization Step 1");
	painter.drawText(_headerText(4.0), "Optimization Step 2");
	painter.drawText(_headerText(6.0), "Tracking");
	painter.drawText(_headerText(7.0), "Detection");
}

void FStatisticsView::_paintStatistics(QPainter& painter)
{
	int x0, y0, w, h;
	const FFrameStatistics& stats = m_statList[m_listIndex];

	painter.setPen(Qt::transparent);

	x0 = m_header.right();
	for (size_t i = 0; i < FTrackerStatistics::NumStages; ++i)
	{
		double mean = stats.tracker.errorMean[i];
		y0 = _rowTop(1.0 + i) + 3;
		w = m_header.right() + _xPos(mean, m_errorScale) - x0 + 1;
		h = _rowBottom(1.0 + i) - 3 - y0;

		int q = (int)fMax(0.0, 120.0 - fMax(0.0, mean - 0.5) * 50.0);
		painter.setBrush(QColor::fromHsv(q, 240, 210));
		painter.drawRect(x0, y0, w, h);
	}

	painter.setPen(QColor(248, 223, 82));
	for (size_t i = 0; i < FTrackerStatistics::NumStages; ++i)
	{
		double mean = stats.tracker.errorMean[i];
		int meanPos = m_header.right() + _xPos(mean, m_errorScale);
		int sdPos = m_header.right() + _xPos(mean + stats.tracker.errorSD[i], m_errorScale);
		y0 = _rowTop(1.0 + i) + 3;
		int y1 = _rowBottom(1.0+ i) - 4;
		int y2 = (y0 + y1) / 2;
		painter.drawLine(meanPos, y2, sdPos, y2);
		painter.drawLine(sdPos, y0, sdPos, y1);
	}

	painter.setPen(Qt::transparent);
	painter.setBrush(QColor(255, 245, 176));
	for (size_t i = 0; i < FTrackerStatistics::NumStages; ++i)
	{
		y0 = _rowTop(1.0 + i) + 1;
		x0 = m_header.right() + _xPos(stats.tracker.errorMedian[i], m_errorScale);
		h = _rowBottom(1.0 + i) - 1 - y0;
		painter.drawRect(x0, y0, 2, h);
	}

	// time bar chart
	x0 = m_header.right();
	y0 = _rowTop(6.0) + 3;
	h = _rowBottom(6.0) - 3 - y0;
	int x1, x2, x3, x4;

	x1 = x0 + _xPos(stats.tracker.timeSearch * 1000.0, m_timeScale);
	x2 = x1 + _xPos(stats.tracker.timeOptimizationA * 1000.0, m_timeScale);
	x3 = x2 + _xPos(stats.tracker.timeOptimizationB * 1000.0, m_timeScale);
	painter.setBrush(QColor(226, 174, 139));
	painter.drawRect(x0, y0, x1 - x0 + 1, h);
	painter.setBrush(QColor(132, 221, 136));
	painter.drawRect(x1, y0, x2 - x1 + 1, h);
	painter.setBrush(QColor(167, 143, 209));
	painter.drawRect(x2, y0, x3 - x2 + 1, h);

	x0 = m_header.right();
	y0 = _rowTop(7.0) + 3;
	h = _rowBottom(7.0) - 3 - y0;

	x1 = x0 + _xPos(stats.detector.timeContourExtraction * 1000.0, m_timeScale);
	x2 = x1 + _xPos(stats.detector.timeContourNormalization * 1000.0, m_timeScale);
	x3 = x2 + _xPos(stats.detector.timeContourAlignment * 1000.0, m_timeScale);
	x4 = x3 + _xPos(stats.detector.timePoseReconstruction * 1000.0, m_timeScale);
	painter.setBrush(QColor(226, 174, 139));
	painter.drawRect(x0, y0, x1 - x0 + 1, h);
	painter.setBrush(QColor(132, 221, 136));
	painter.drawRect(x1, y0, x2 - x1 + 1, h);
	painter.setBrush(QColor(167, 143, 209));
	painter.drawRect(x2, y0, x3 - x2 + 1, h);
	painter.setBrush(QColor(226, 174, 139));
	painter.drawRect(x3, y0, x4 - x3 + 1, h);

	// relative pose change
	static const QColor col[7] = { QColor(255, 0, 0), QColor(0, 255, 0), QColor(0, 0, 255),
		QColor(200, 200, 100), QColor(100, 200, 200), QColor(200, 100, 200), QColor(255, 255, 255) };

	for (int i = 0; i < 50; i++)
	{
		int i0 = (m_listIndex + HISTORY_SIZE - i - 2) % HISTORY_SIZE;
		int i1 = (m_listIndex + HISTORY_SIZE - i - 1) % HISTORY_SIZE;
		int i2 = (m_listIndex + HISTORY_SIZE - i) % HISTORY_SIZE;

		int x0 = m_header.right() + _xPos(49 - i, FVector2d(0.0, 50.0));
		int x1 = m_header.right() + _xPos(50 - i, FVector2d(0.0, 50.0));
		int y0 = _rowBottom(15.0f);

		for (int j = 0; j < 7; j++)
		{
			double v0 = m_statList[i0].finalPose[j];
			double v1 = m_statList[i1].finalPose[j];
			double v2 = m_statList[i2].finalPose[j];

			double d0 = fMin(7.0 * m_rowHeight, fabs(0.5 * m_graphHeight * (v1 - v0)));
			double d1 = fMin(7.0 * m_rowHeight, fabs(0.5 * m_graphHeight * (v2 - v1)));

			painter.setPen(col[j]);
			painter.drawLine(x0, y0 - d0, x1, y0 - d1);
		}
	}

	// tracker state
	painter.drawText(_contentText(8.0),
		QString("Detected Contours: %1,  Tracker State: %2")
		.arg(stats.detector.numPoses)
		.arg(stats.tracker.state.toString()));

	// variance bars
	painter.setPen(Qt::transparent);
	painter.setBrush(QColor(120, 144, 180));
	QPoint pos = _headerText(9.0) + QPoint(110, 0);
	double fw = (m_header.right() - pos.x()) / 0.2;
	painter.drawRect(pos.x(), pos.y() - 11,
		fMin(0.2, m_statList[m_listIndex].tracker.variance[0]) * fw, 14);

	for (int i = 0; i < 6; i++)
	{
		pos = _headerText(10.0 + (float)i) + QPoint(110, 0);
		painter.drawRect(pos.x(), pos.y() - 11,
			fMin(0.2, m_statList[m_listIndex].tracker.variance[1 + i]) * fw, 14);
	}

	painter.setPen(QColor(240, 240, 240));

	// current pose values
	pos = _headerText(9.0);
	painter.drawText(pos, "Pos X");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[0], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[0], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[0], 'f', 4));

	pos = _headerText(10.0);
	painter.drawText(pos, "Pos Y");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[1], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[1], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[1], 'f', 4));
	
	pos = _headerText(11.0);
	painter.drawText(pos, "Pos Z");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[2], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[2], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[2], 'f', 4));
	
	pos = _headerText(12.0);
	painter.drawText(pos, "Rot X");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[3], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[3], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[3], 'f', 4));	
	
	pos = _headerText(13.0);
	painter.drawText(pos, "Rot Y");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[4], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[4], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[4], 'f', 4));

	pos = _headerText(14.0);
	painter.drawText(pos, "Rot Z");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[5], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[5], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[5], 'f', 4));
	
	pos = _headerText(15.0);
	painter.drawText(pos, "Lens");
	pos += QPoint(45, 0);
	painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPose[6], 'f', 2));
	pos += QPoint(65, 0);
	//painter.drawText(pos, QString::number(m_statList[m_listIndex].finalPoseSmooth[6], 'f', 2));
	painter.drawText(pos, QString::number(m_statList[m_listIndex].tracker.variance[6], 'f', 4));
}

int FStatisticsView::_rowTop(float row)
{
	return m_header.top() + row * m_rowHeight;
}

int FStatisticsView::_rowBottom(float row)
{
	return m_header.top() + row * m_rowHeight + m_rowHeight - 1;
}

QRect FStatisticsView::_headerRow(float row)
{
	int top = m_header.top() + row * m_rowHeight;
	return QRect(m_header.left(), top, m_header.width(), m_rowHeight - 1);
}

QRect FStatisticsView::_contentRow(float row)
{
	int left = m_header.right();
	int top = m_header.top() + row * m_rowHeight;
	return QRect(left, top, width() - left - m_rightMargin, m_rowHeight - 1);
}

QRect FStatisticsView::_fullRow(float row)
{
	int top = m_header.top() + row * m_rowHeight;
	return QRect(m_header.left(), top, width() - m_header.left() - m_rightMargin, m_rowHeight - 1);
}

QPoint FStatisticsView::_headerText(float row)
{
	return QPoint(m_header.left() + 3, m_header.top() + row * m_rowHeight + 13);
}

QPoint FStatisticsView::_contentText(float row)
{
	return QPoint(m_header.right() + 3, m_header.top() + row * m_rowHeight + 13);
}

int FStatisticsView::_xPos(double val, const FVector2d& bounds)
{
	double width = this->width() - m_rightMargin - m_header.right();
	return (int)((val - bounds.x()) / (bounds.y() - bounds.x()) * width + 0.5);
}

// ----------------------------------------------------------------------------------------------------