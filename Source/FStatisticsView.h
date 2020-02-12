// ----------------------------------------------------------------------------------------------------
//  Title			FStatisticsView.h
//  Description		Header file for FStatisticsView.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTATISTICSVIEW_H
#define FSTATISTICSVIEW_H

#include <QWidget>
#include "FTrackMe.h"
#include "FlowMath.h"
#include "FFrameStatistics.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStatisticsView
// ----------------------------------------------------------------------------------------------------

class FStatisticsView : public QWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStatisticsView(QWidget* pParent = NULL, Qt::WindowFlags flags = 0);
	/// Virtual destructor.
	virtual ~FStatisticsView();

	//  Public commands --------------------------------------------------------

public:
	void startLog(const QString& logFilePath);
	void stopLog();

	//  Public queries ---------------------------------------------------------

	//  Public slots -----------------------------------------------------------

public slots:
	void updateStatistics(FFrameStatistics stats);

	//  Overrides --------------------------------------------------------------

protected:
	void paintEvent(QPaintEvent* pPaintEvent);
	void resizeEvent(QResizeEvent* pResizeEvent);
	QSize sizeHint() const;

	//  Internal functions -----------------------------------------------------

private:
	void _writeLog(const FFrameStatistics& stats);
	void _paintFramework(QPainter& painter);
	void _paintStatistics(QPainter& painter);

	int _rowTop(float row);
	int _rowBottom(float row);
	QRect _headerRow(float row);
	QRect _contentRow(float row);
	QRect _fullRow(float row);
	QPoint _headerText(float row);
	QPoint _contentText(float row);
	int _xPos(double val, const FVector2d& bounds);

	//  Internal data members --------------------------------------------------

private:
	QRect m_header;
	float m_rowHeight;
	int m_rightMargin;
	int m_graphHeight;

	FVector2d m_errorScale;
	double m_errorInterval;

	FVector2d m_timeScale;
	double m_timeInterval;

	// data
	static const size_t HISTORY_SIZE = 128;
	size_t m_listIndex;
	std::vector<FFrameStatistics> m_statList;

	double m_timeSum[7];

	// log file
	QString m_logFilePath;
	QFile* m_pLogFile;
	QTextStream* m_pLogStream;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTATISTICSVIEW_H