// ----------------------------------------------------------------------------------------------------
//  Title			FMainWindow.h
//  Description		Header file for FMainWindow.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FMAINWINDOW_H
#define FMAINWINDOW_H

#include "FTrackMe.h"
#include "FMainWindowBase.h"

class FProcessingThread;
class FStreamSource;
class FParameterController;
class FStreamViewerFrame;
class FStatisticsView;
class FTrainingWindow;
class FDialogAbout;

// ----------------------------------------------------------------------------------------------------
//  Class FMainWindow
// ----------------------------------------------------------------------------------------------------

class FMainWindow : public FMainWindowBase
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FMainWindow(QWidget* pParent = NULL, Qt::WindowFlags flags = 0);
	/// Virtual destructor.
	virtual ~FMainWindow();

	//  Public commands --------------------------------------------------------

public:

	//  Public queries ---------------------------------------------------------


	//  Overrides --------------------------------------------------------------

protected:
	virtual void timerEvent(QTimerEvent* pTimerEvent);
	virtual void closeEvent(QCloseEvent* pCloseEvent);
	virtual QMenu* onCreateMenu(QMenuBar* pMenuBar);
	virtual void onCreateDockWidgets(QMenu* pMenuView = NULL);

protected slots:
	void onOpenProject();
	void onSaveProject();
	void onSaveProjectAs();
	void onOpenMediaFile();
	void onOpenMediaStream(QAction* pMenuItem);
	void onStreamProperties();
	void onStartPlayout();
	void onStopPlayout();
	void onStartStatisticsLog();
	void onStopStatisticsLog();
	void onTraining();
	void onShowAbout();

signals:
	void openMediaFile(QString filePath);
	void openMediaStream(int ordinal, QSize imageSize);
	void startPlayout(QString filePath);
	void stopPlayout();

	//  Internal functions -----------------------------------------------------

private:
	void _createStreamProcessor();

	//  Internal data members --------------------------------------------------

private:
	QMenu* m_pMenuStreamSource;
	QStringList m_captureDevices;
	FProcessingThread* m_pThread;
	FStreamSource* m_pStreamSource;
	FParameterController* m_pController;
	FStreamViewerFrame* m_pStreamViewer;
	FStatisticsView* m_pStatView;
	FTrainingWindow* m_pTrainingWindow;
	FDialogAbout* m_pDialogAbout;

	int m_timerId;
	QString m_currentProjectFile;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FMAINWINDOW_H