// ----------------------------------------------------------------------------------------------------
//  Title			FProcessingThread.cpp
//  Description		Implementation of class FProcessingThread
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 9 $
//  $Date: 2011-08-14 21:08:00 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FStreamProcessor.h"

#include <ObjBase.h>

#include "FProcessingThread.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FProcessingThread
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FProcessingThread::FProcessingThread(QObject* pParent /* = NULL */)
: QThread(pParent),
  m_pProcessor(NULL),
  m_pRenderWidget(NULL),
  m_wantExit(false),
  m_isInitialized(false)
{
}

FProcessingThread::~FProcessingThread()
{
}

// Public commands ------------------------------------------------------------------------------------

void FProcessingThread::start(QThread::Priority priority /* = QThread::NormalPriority */)
{
	F_ASSERT(m_pRenderWidget);
	if (!m_pRenderWidget)
		return;

	QThread::start(priority);
}

void FProcessingThread::stop()
{
	if (isRunning())
	{
		m_wantExit = true;
		QThread::wait(ULONG_MAX);
	}
}

void FProcessingThread::setRenderWidget(FRenderWidget* pRenderWidget)
{
	F_ASSERT(pRenderWidget);
	m_pRenderWidget = pRenderWidget;
}

// Public queries -------------------------------------------------------------------------------------

FStreamSource* FProcessingThread::streamSource() const
{
	return m_pProcessor->streamSource();
}

FStreamEngine* FProcessingThread::streamEngine() const
{
	return m_pProcessor->streamEngine();
}

FStreamViewer* FProcessingThread::streamViewer() const
{
	return m_pProcessor->streamViewer();
}

// Overrides ------------------------------------------------------------------------------------------

void FProcessingThread::run()
{
	bool keepRunning = true;
	CoInitializeEx(NULL, 0);
	m_pProcessor = new FStreamProcessor();

	F_ASSERT(m_pRenderWidget);
	if (!m_pProcessor->initialize(m_pRenderWidget))
	{
		F_ASSERT(false);
		F_SAFE_DELETE(m_pProcessor);
		return;
	}
	
	m_isInitialized = true;

	while (!m_wantExit && keepRunning)
	{
		keepRunning = m_pProcessor->process();
		QThread::msleep(0);
		QAbstractEventDispatcher::instance()->processEvents(QEventLoop::AllEvents);
	}

	m_isInitialized = false;
	m_wantExit = false;
	F_SAFE_DELETE(m_pProcessor);

	CoUninitialize();
}

// Internal functions ---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------