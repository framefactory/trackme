// ----------------------------------------------------------------------------------------------------
//  Title			FMainWindow.cpp
//  Description		Implementation of class FMainWindow
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FLogWidget.h"

#include "TrackMe.Version.h"

#include "FGLContext.h"
#include "FProcessingThread.h"
#include "FStreamProcessor.h"
#include "FStreamSource.h"
#include "FParameterController.h"
#include "FStreamViewerFrame.h"
#include "FRenderWidget.h"
#include "FStatisticsView.h"
#include "FTrainingWindow.h"
#include "FDialogAbout.h"
#include "FArchive.h"

#include "FMainWindow.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FMainWindow
// ----------------------------------------------------------------------------------------------------

// Static members -------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FMainWindow::FMainWindow(QWidget* pParent /* = NULL */, Qt::WindowFlags flags /* = 0 */)
: FMainWindowBase(pParent, flags),
  m_pTrainingWindow(NULL),
  m_pDialogAbout(NULL)
{
#ifdef QT_DEBUG
	QString config = "Debug";
#else
	QString config = "Release";
#endif

	setWindowTitle(QString("TrackMe - Version %1.%2 - %3 Build %4")
		.arg(FGlobalConstants::VERSION_MAJOR).arg(FGlobalConstants::VERSION_MINOR)
		.arg(config).arg(TRACKME_BUILD));

	FGLContext::initGlew();

	initializeUserInterface(QSize(1400, 1100));
	setCorner(Qt::BottomLeftCorner, Qt::LeftDockWidgetArea);

	m_pStreamViewer = new FStreamViewerFrame(this);
	setCentralWidget(m_pStreamViewer);

	_createStreamProcessor();
	m_timerId = startTimer(200);
}

FMainWindow::~FMainWindow()
{
	m_pThread->stop();
	F_SAFE_DELETE(m_pThread);
}

// Public queries -------------------------------------------------------------------------------------

// Protected slots ------------------------------------------------------------------------------------

void FMainWindow::onOpenProject()
{
	QString filePath = QFileDialog::getOpenFileName(
		this, "Open Project File", QString(), "Projects (*.tpr)");

	if (filePath.isEmpty())
		return;

	QFile m_file(filePath);
	if (!m_file.open(QIODevice::ReadOnly))
		return;

	FArchive archive(&m_file, false);
	
	int srcType;
	archive >> srcType;
	QString srcPath;
	archive >> srcPath;
	int srcOrdinal;
	archive >> srcOrdinal;
	if (srcType == (int)FStreamSource::Image
			|| srcType == (int)FStreamSource::ImageSequence
			|| srcType == (int)FStreamSource::Movie)
		emit openMediaFile(srcPath);
	else
		emit openMediaStream(srcOrdinal, QSize(640, 480));
	
	m_pController->read(archive);
	m_currentProjectFile = filePath;

	fInfo("Project", QString("Successfully read from %1").arg(m_currentProjectFile));
}

void FMainWindow::onSaveProject()
{
	if (m_currentProjectFile.isEmpty())
		return;

	QFile m_file(m_currentProjectFile);
	if (!m_file.open(QIODevice::WriteOnly))
		return;

	FArchive archive(&m_file, false);

	archive << (int)m_pStreamSource->sourceType();
	archive << m_pStreamSource->sourcePath();
	archive << (int)m_pStreamSource->streamOrdinal();

	m_pController->write(archive);

	fInfo("Project", QString("Successfully written to %1").arg(m_currentProjectFile));
}

void FMainWindow::onSaveProjectAs()
{
	QString filePath = QFileDialog::getSaveFileName(
		this, "Save Project File", QString(), "Projects (*.tpr)");

	if (filePath.isEmpty())
		return;

	m_currentProjectFile = filePath;
	onSaveProject();
}

void FMainWindow::onOpenMediaFile()
{
	QString filePath = QFileDialog::getOpenFileName(
		this, "Open Media File", QString(),
		"Images (*.tif *.jpg *.png *.bmp);;Movies (*.avi *.mp4 *.mov)");

	if (!filePath.isEmpty())
		emit openMediaFile(filePath);
}

void FMainWindow::onOpenMediaStream(QAction* pMenuItem)
{
	if (!pMenuItem)
		return;

	for (int i = 0; i < m_captureDevices.size(); ++i)
	{
		if (m_captureDevices[i] == pMenuItem->text())
			emit openMediaStream(i, QSize(640, 480));
	}
}

void FMainWindow::onStreamProperties()
{
	m_pThread->streamProcessor()->showStreamProperties();
}

void FMainWindow::onStartPlayout()
{
	QString filePath = QFileDialog::getSaveFileName(
		this, "Select Playout File", QString(), "JPEG Images (*.jpg)");

	if (!filePath.isEmpty())
		emit startPlayout(filePath);
}

void FMainWindow::onStopPlayout()
{
	emit stopPlayout();
}

void FMainWindow::onStartStatisticsLog()
{
	QString filePath = QFileDialog::getSaveFileName(
		this, "Select Log File", QString(), "Text Files (*.txt)");

	if (!filePath.isEmpty())
		m_pStatView->startLog(filePath);
}

void FMainWindow::onStopStatisticsLog()
{
	m_pStatView->stopLog();
}

void FMainWindow::onTraining()
{
	F_SAFE_DELETE(m_pTrainingWindow);
	m_pTrainingWindow = new FTrainingWindow(this);
	m_pTrainingWindow->show();
}

void FMainWindow::onShowAbout()
{
	F_SAFE_DELETE(m_pDialogAbout);
	m_pDialogAbout = new FDialogAbout(this);
	m_pDialogAbout->show();
}

// Overrides ------------------------------------------------------------------------------------------

void FMainWindow::timerEvent(QTimerEvent* pTimerEvent)
{
	if (pTimerEvent->timerId() != m_timerId)
		return;

	killTimer(pTimerEvent->timerId());
	m_pController->activatePreset(0);
}

void FMainWindow::closeEvent(QCloseEvent* pCloseEvent)
{
	pCloseEvent->accept();
	saveWindowState();
}

QMenu* FMainWindow::onCreateMenu(QMenuBar* pMenuBar)
{
	QMenu* pMenuFile = pMenuBar->addMenu("&File");

	pMenuFile->addAction("Open Project...", this, SLOT(onOpenProject()));
	pMenuFile->addAction("Save Project", this, SLOT(onSaveProject()));
	pMenuFile->addAction("Save Project As...", this, SLOT(onSaveProjectAs()));
	pMenuFile->addSeparator();
	pMenuFile->addAction("&Open File...", this, SLOT(onOpenMediaFile()), QKeySequence("Ctrl+O"));
	m_pMenuStreamSource = pMenuFile->addMenu("Open S&tream");
	connect(m_pMenuStreamSource, SIGNAL(triggered(QAction*)), this, SLOT(onOpenMediaStream(QAction*)));
	pMenuFile->addAction("Stream Properties...", this, SLOT(onStreamProperties()));
	pMenuFile->addSeparator();
	pMenuFile->addAction("Start Playout...", this, SLOT(onStartPlayout()));
	pMenuFile->addAction("Stop Playout", this, SLOT(onStopPlayout()));
	pMenuFile->addSeparator();
	pMenuFile->addAction("Start Statistics Log...", this, SLOT(onStartStatisticsLog()));
	pMenuFile->addAction("Stop Statistics Log", this, SLOT(onStopStatisticsLog()));
	pMenuFile->addSeparator();
	pMenuFile->addAction("Training...", this, SLOT(onTraining()), QKeySequence("Ctrl+T"));
	pMenuFile->addSeparator();
	pMenuFile->addAction("&Quit", this, SLOT(close()), QKeySequence("Ctrl+Q"));

	QMenu* pMenuEdit = pMenuBar->addMenu("&Edit");

	pMenuEdit->addAction("&Run");
	pMenuEdit->addAction("&Stop");
	pMenuEdit->addSeparator();
	pMenuEdit->addAction("&Next Frame");
	pMenuEdit->addAction("&Previous Frame");
	pMenuEdit->addSeparator();
	pMenuEdit->addAction("To &Begin");
	pMenuEdit->addAction("To &End");

	QMenu* pMenuView = pMenuBar->addMenu("&View");

	QMenu* pMenuHelp = pMenuBar->addMenu("&Help");

	pMenuHelp->addAction("&About TrackMe...", this, SLOT(onShowAbout()));

	return pMenuView;
}

void FMainWindow::onCreateDockWidgets(QMenu* pMenuView /* = NULL */)
{
	m_pController = new FParameterController(this);
	addDockWidget(m_pController, "Parameters", Qt::LeftDockWidgetArea, pMenuView);

	m_pStatView = new FStatisticsView(this);
	addDockWidget(m_pStatView, "Statistics", Qt::RightDockWidgetArea, pMenuView);

	FLogWidget* pLogWindow = new FLogWidget(this);
	addDockWidget(pLogWindow, "Log", Qt::BottomDockWidgetArea, pMenuView);
}

// Internal functions ---------------------------------------------------------------------------------

void FMainWindow::_createStreamProcessor()
{
	WId id = m_pStreamViewer->renderWidget()->winId();
	m_pThread = new FProcessingThread(this);
	m_pThread->setRenderWidget(m_pStreamViewer->renderWidget());
	m_pThread->start();

	while(!m_pThread->isInitialized())
		::Sleep(100);

	FStreamProcessor* pProcessor = m_pThread->streamProcessor();
	FStreamEngine* pEngine = m_pThread->streamEngine();
	m_pStreamSource = m_pThread->streamSource();

	m_captureDevices = m_pStreamSource->captureDeviceList();
	for (int i = 0; i < m_captureDevices.size(); ++i)
		m_pMenuStreamSource->addAction(m_captureDevices[i]);

	m_pController->setStreamProcessor(pProcessor);
	m_pController->setStreamEngine(pEngine);

	connect(this, SIGNAL(openMediaFile(QString)),
		pProcessor, SLOT(openMediaFile(QString)));
	connect(this, SIGNAL(openMediaStream(int, QSize)),
		pProcessor, SLOT(openMediaStream(int, QSize)));
	connect(this, SIGNAL(startPlayout(QString)),
		pProcessor, SLOT(startPlayout(QString)));
	connect(this, SIGNAL(stopPlayout()),
		pProcessor, SLOT(stopPlayout()));
	connect(this, SIGNAL(keyPressed(FKeyboardState)),
		pProcessor, SLOT(keyPressed(FKeyboardState)));

	connect(m_pStreamViewer, SIGNAL(buttonPlay()), pProcessor, SLOT(playMedia()));
	connect(m_pStreamViewer, SIGNAL(buttonStop()), pProcessor, SLOT(stopMedia()));
	connect(m_pStreamViewer, SIGNAL(buttonPrevious()), pProcessor, SLOT(previousFrame()));
	connect(m_pStreamViewer, SIGNAL(buttonNext()), pProcessor, SLOT(nextFrame()));
	connect(m_pStreamViewer, SIGNAL(buttonBegin()), pProcessor, SLOT(firstFrame()));
	connect(m_pStreamViewer, SIGNAL(buttonEnd()), pProcessor, SLOT(lastFrame()));

	connect(m_pStreamViewer, SIGNAL(viewModeChanged(quint32)), pProcessor, SLOT(setViewMode(quint32)));
	connect(m_pStreamViewer, SIGNAL(playbackSpeedChanged(float)), pProcessor, SLOT(setPlaybackSpeed(float)));

	connect(pProcessor, SIGNAL(mediaStateChanged(QString)), m_pStreamViewer, SLOT(setMediaInfo(QString)));
	connect(pProcessor, SIGNAL(frameSizeChanged(QSize)), m_pStreamViewer, SLOT(changeResolution(QSize)));

	connect(pProcessor, SIGNAL(statisticsUpdated(FFrameStatistics)),
		m_pStatView, SLOT(updateStatistics(FFrameStatistics)));
}

// ----------------------------------------------------------------------------------------------------