// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingWindow.cpp
//  Description		Implementation of class FTrainingWindow
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-04 18:33:19 +0200 (So, 04 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FTrainingEngine.h"
#include "FTrainingView.h"
#include "FTrainingParameterController.h"

#include "FTrainingWindow.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingWindow
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FTrainingWindow::FTrainingWindow(QWidget* pParent)
: FMainWindowBase(pParent),
  m_pInfoWidget(NULL),
  m_trainingRunning(false),
  m_testRunning(false),
  m_trainingTimer(0),
  m_testTimer(0),
  m_testRun(0)
{
	initializeUserInterface(QSize(1380, 980), true);
	setWindowTitle("TrackMe - Contour Training");

	m_pTrainingEngine = new FTrainingEngine(m_pTrainingView->renderWidget());

	connect(m_pTrainingEngine, SIGNAL(postMessage(QString)), this, SLOT(postMessage(QString)));
	connect(m_pTrainingEngine, SIGNAL(postDatabaseInfo(QString)), this, SLOT(postDatabaseInfo(QString)));
	connect(m_pTrainingEngine, SIGNAL(postStatistics(FTrainingStatistics)),
		m_pTrainingView, SLOT(updateStatistics(FTrainingStatistics)));

	m_pController->setEngine(m_pTrainingEngine);
}

FTrainingWindow::~FTrainingWindow()
{
	F_SAFE_DELETE(m_pInfoWidget);
	F_SAFE_DELETE(m_pTrainingEngine);
}

// Public slots ---------------------------------------------------------------------------------------

void FTrainingWindow::onLoadContourModel()
{
	QString filePath = QFileDialog::getOpenFileName(
		this, "Open Contour Model", QString(), "Collada (*.dae)");

	if (filePath.isEmpty())
		return;

	m_pTrainingEngine->loadContourModel(filePath);
	m_pTrainingEngine->updateView();
}

void FTrainingWindow::onLoadData()
{
	QString filePath = QFileDialog::getOpenFileName(
		this, "Open Training Data File", QString(), "TrackMe Training (*.tmt)");

	if (filePath.isEmpty())
		return;

	m_pTrainingEngine->loadClassifierData(filePath);
}

void FTrainingWindow::onSaveData()
{
	QString filePath = QFileDialog::getSaveFileName(
		this, "Save Training Data File", QString(), "TrackMe Training (*.tmt)");

	if (filePath.isEmpty())
		return;

	m_pTrainingEngine->saveClassifierData(filePath);
}

void FTrainingWindow::onRunTraining()
{
	if (m_testRunning)
		return;

	if (!m_trainingRunning)
	{
		m_trainingTimer = startTimer(50);
		m_pActionTraining->setIcon(QIcon(":/icons/media_stop_16"));
		m_trainingRunning = true;
	}
	else
	{
		killTimer(m_trainingTimer);
		m_pActionTraining->setIcon(QIcon(":/icons/media_play_16"));
		m_trainingRunning = false;
	}
}

void FTrainingWindow::onTrainingStep()
{
	if (m_testRunning)
		return;

	m_pTrainingEngine->runOnce();
	m_pTrainingEngine->updateView();
}

void FTrainingWindow::onRunTest()
{
	if (m_trainingRunning)
		return;

	if (!m_testRunning)
	{
		m_testTimer = startTimer(10);
		m_protocolFile.setFileName("pose_verification_test.txt");
		m_protocolFile.open(QIODevice::WriteOnly);
		m_protocolStream.setDevice(&m_protocolFile);
		m_testRun = 0;
	}
	else
	{
		killTimer(m_testTimer);
		m_protocolStream.flush();
		m_protocolFile.close();
	}

	m_testRunning = !m_testRunning;
}

void FTrainingWindow::onDatabaseInfo()
{
	F_SAFE_DELETE(m_pInfoWidget);
	QString info = m_pTrainingEngine->databaseInfo();
	m_pInfoWidget = new QWidget();

	QTextEdit* pTextEdit = new QTextEdit(m_pInfoWidget);
	pTextEdit->setReadOnly(true);
	pTextEdit->setFontFamily("Lucida Console");
	pTextEdit->setFontPointSize(10.0f);
	pTextEdit->setText(info);

	QHBoxLayout* pLayout = new QHBoxLayout(m_pInfoWidget);
	pLayout->setMargin(0);
	pLayout->addWidget(pTextEdit);

	m_pInfoWidget->setLayout(pLayout);
	m_pInfoWidget->resize(960, 960);
	m_pInfoWidget->show();
}

void FTrainingWindow::postMessage(QString text)
{
	QDateTime now = QDateTime::currentDateTime();
	QString itemText = QString("%1  %2").arg(now.toString("dd.MM.yyyy  hh:mm:ss")).arg(text);
	QTreeWidgetItem* pItem = new QTreeWidgetItem(QStringList(itemText));
	m_pLogList->addTopLevelItem(pItem);
	m_pLogList->setCurrentItem(pItem);
}

void FTrainingWindow::postDatabaseInfo(QString text)
{
	m_pLabelDatabaseInfo->setText(text);
}

// Overrides ------------------------------------------------------------------------------------------

void FTrainingWindow::closeEvent(QCloseEvent* pCloseEvent)
{
	pCloseEvent->accept();
}

void FTrainingWindow::timerEvent(QTimerEvent* pTimerEvent)
{
	if (pTimerEvent->timerId() == m_trainingTimer)
	{
		m_pTrainingEngine->runOnce();
		m_pTrainingEngine->updateView();
	}
	else if (pTimerEvent->timerId() == m_testTimer)
	{
		m_pTrainingEngine->testOnce(m_protocolStream);
		m_pTrainingEngine->updateView();

		m_testRun++;
		if (m_testRun >= MAX_RUNS)
			onRunTest();
	}
}

QMenu* FTrainingWindow::onCreateMenu(QMenuBar *pMenuBar)
{
	QMenu* pMenuView = menuBar()->addMenu("&View");
	return pMenuView;
}

void FTrainingWindow::onCreateDockWidgets(QMenu* pMenuView)
{
	m_pController = new FTrainingParameterController(this);
	addDockWidget(m_pController, "Parameters", Qt::TopDockWidgetArea, pMenuView);

	m_pTrainingView = new FTrainingView(this);
	addDockWidget(m_pTrainingView, "Training View", Qt::TopDockWidgetArea, pMenuView);

	m_pLogList = new QTreeWidget(this);
	m_pLogList->setIndentation(0);
	m_pLogList->setWordWrap(true);
	m_pLogList->setRootIsDecorated(false);

	QStringList headerLabels;
	headerLabels << tr("Message");
	m_pLogList->setHeaderLabels(headerLabels);

	QDockWidget* pDW = addDockWidget(m_pLogList, "Training Log", Qt::BottomDockWidgetArea);
	pDW->resize(100, 50);
	pDW->setVisible(false);
}

void FTrainingWindow::onCreateToolbar()
{
	QToolBar* pToolBar = addToolBar("Training");

	pToolBar->addAction(QIcon(":/icons/shape_open_16"), "Load Contour Model", this, SLOT(onLoadContourModel()));

	pToolBar->addSeparator();
	pToolBar->addAction(QIcon(":/icons/document_open_16"), "Load Classifier Data", this, SLOT(onLoadData()));
	pToolBar->addAction(QIcon(":/icons/document_save_16"), "Save Classifier Data", this, SLOT(onSaveData()));

	pToolBar->addSeparator();
	m_pActionTraining = pToolBar->addAction(QIcon(":/icons/media_play_16"), "Start/Stop Training", this, SLOT(onRunTraining()));
	pToolBar->addAction(QIcon(":/icons/media_next_16"), "Train One Sample", this, SLOT(onTrainingStep()));

	pToolBar->addSeparator();
	pToolBar->addAction(QIcon(":/icons/stopwatch_16"), "Run Test Statistics", this, SLOT(onRunTest()));
	pToolBar->addAction(QIcon(":/icons/document_16"), "Database Info", this, SLOT(onDatabaseInfo()));

	pToolBar->addSeparator();
	m_pLabelDatabaseInfo = new QLabel(this);
	m_pLabelDatabaseInfo->setAttribute(Qt::WA_NoSystemBackground);
	m_pLabelDatabaseInfo->setWordWrap(false);
	pToolBar->addWidget(m_pLabelDatabaseInfo);
}

// ----------------------------------------------------------------------------------------------------