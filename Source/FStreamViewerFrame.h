// ----------------------------------------------------------------------------------------------------
//  Title			FStreamViewerFrame.h
//  Description		Header file for FStreamViewerFrame.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMVIEWERFRAME_H
#define FSTREAMVIEWERFRAME_H

#include <QWidget>
#include "FTrackMe.h"
#include "FAspect.h"
#include "FStreamViewMode.h"

class FRenderScrollArea;
class FRenderWidget;

// ----------------------------------------------------------------------------------------------------
//  Class FStreamViewerFrame
// ----------------------------------------------------------------------------------------------------

class FStreamViewerFrame : public QWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamViewerFrame(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamViewerFrame();

	//  Public commands --------------------------------------------------------

public:

public slots:
	void setMediaInfo(QString info);
	void changeResolution(QSize resolution);

protected slots:
	void onComboResolution(int index);
	void onComboSpeed(int index);
	void onComboView(int index);

	//  Public queries ---------------------------------------------------------

public:
	/// Returns the widget on which the rendered graphics are drawn.
	FRenderWidget* renderWidget() const;
	/// Returns the render widget's scroll area.
	FRenderScrollArea* renderScrollArea() const;

	//  Signals ----------------------------------------------------------------

signals:
	void buttonPlay();
	void buttonStop();
	void buttonPrevious();
	void buttonNext();
	void buttonBegin();
	void buttonEnd();

	void viewModeChanged(quint32 mode);
	void playbackSpeedChanged(float fps);

	//  Overrides --------------------------------------------------------------

protected:

	//  Internal functions -----------------------------------------------------

private:
	void _initialize();
	void _initSpeedList();
	void _initResolutionList();
	void _initViewList();

	//  Internal data members --------------------------------------------------

private:
	FRenderScrollArea* m_pRenderArea;
	QToolBar* m_pToolbar;

	QComboBox* m_pComboSpeed;
	QList<float> m_speedList;
	QComboBox* m_pComboResolution;
	QList<FAspect> m_resolutionList;
	QComboBox* m_pComboView;

	QLabel* m_pLabelStatus;

	QAction* m_pActPlay;
	QAction* m_pActStop;
	QAction* m_pActPrev;
	QAction* m_pActNext;
	QAction* m_pActBegin;
	QAction* m_pActEnd;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMVIEWERFRAME_H