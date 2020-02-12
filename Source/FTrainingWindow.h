// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingWindow.h
//  Description		Header file for FTrainingWindow.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-04 18:33:19 +0200 (So, 04 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGWINDOW_H
#define FTRAININGWINDOW_H

#include "FTrackMe.h"
#include "FMainWindowBase.h"

class FTrainingEngine;
class FTrainingView;
class QTreeWidget;
class FTrainingParameterController;

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingWindow
// ----------------------------------------------------------------------------------------------------

/// Main window for contour training. Inherits from FMainWindowBase and provides
/// dockable widgets for parameters, training view and log.
class  FTrainingWindow : public FMainWindowBase
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FTrainingWindow(QWidget* pParent);
	/// Virtual destructor.
	virtual ~FTrainingWindow();

	//  Public slots -----------------------------------------------------------

	public slots:
		void onLoadContourModel();
		void onLoadData();
		void onSaveData();
		void onRunTraining();
		void onTrainingStep();
		void onRunTest();
		void onDatabaseInfo();

		void postMessage(QString text);
		void postDatabaseInfo(QString text);

	//  Overrides --------------------------------------------------------------

protected:
	virtual void closeEvent(QCloseEvent* pCloseEvent);
	virtual void timerEvent(QTimerEvent* pTimerEvent);

	virtual QMenu* onCreateMenu(QMenuBar* pMenuBar);
	virtual void onCreateDockWidgets(QMenu* pMenuView);
	virtual void onCreateToolbar();

	//  Internal data members --------------------------------------------------

private:
	FTrainingEngine* m_pTrainingEngine;
	FTrainingView* m_pTrainingView;
	FTrainingParameterController* m_pController;

	QTreeWidget* m_pLogList;
	QLabel* m_pLabelDatabaseInfo;
	QWidget* m_pInfoWidget;
	QAction* m_pActionTraining;

	QFile m_protocolFile;
	QTextStream m_protocolStream;

	bool m_trainingRunning;
	bool m_testRunning;
	int m_trainingTimer;
	int m_testTimer;

	static const size_t MAX_RUNS = 1000;
	size_t m_testRun;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGWINDOW_H