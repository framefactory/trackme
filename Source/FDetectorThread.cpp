// ----------------------------------------------------------------------------------------------------
//  Title			FDetectorThread.cpp
//  Description		Implementation of class FDetectorThread
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/14 $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FPoseDetector.h"

#include "FDetectorThread.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FDetectorThread
// ----------------------------------------------------------------------------------------------------

// Static members -------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FDetectorThread::FDetectorThread(QObject* pParent /* = NULL */)
: QThread(pParent),
  m_wantExit(false),
  m_isIdle(true),
  m_pDetector(NULL),
  m_pDTImage(NULL),
  m_poseCount(0)
{
}

FDetectorThread::~FDetectorThread()
{
}

// Public commands ------------------------------------------------------------------------------------

void FDetectorThread::start(QThread::Priority priority /* = QThread::NormalPriority */)
{
	m_wantExit = false;
	QThread::start(priority);
}

void FDetectorThread::stop()
{
	if (isRunning())
	{
		m_wantExit = true;
		m_frameAvailable.wakeAll();
		QThread::wait(ULONG_MAX);
	}
}

void FDetectorThread::processFrame(FDTPixel* pDistanceTransform)
{
	if (!isRunning())
	{
		F_ASSERT(false);
		return;
	}

	m_objectLock.lock();
	m_isIdle = false;
	m_pDTImage = pDistanceTransform;
	m_objectLock.unlock();

	m_frameAvailable.wakeAll();
}

void FDetectorThread::setPoseDetector(FPoseDetector* pDetector)
{
	m_objectLock.lock();
	m_pDetector = pDetector;
	m_objectLock.unlock();
}

// Overrides ------------------------------------------------------------------------------------------

void FDetectorThread::run()
{
	fInfo("Pose Detector", "Thread started");
	m_objectLock.lock();
	
	while (true)
	{
		m_frameAvailable.wait(&m_objectLock);

		if (m_wantExit)
		{
			m_objectLock.unlock();
			return;
		}

		_processFrame();
		m_isIdle = true;
	}
}

// Internal functions ---------------------------------------------------------------------------------

void FDetectorThread::_processFrame()
{
	F_ASSERT(m_pDTImage);
	F_ASSERT(m_pDetector);

	if (!m_pDetector || !m_pDTImage)
		return;

	m_pDetector->detect(m_pDTImage, &m_statistics);

	m_poseDataLock.lock();
	m_poseCount = m_pDetector->poseCount();
	for (size_t i = 0; i < m_poseCount; i++)
		m_detectedPose[i] = m_pDetector->detectedPose(i);
	m_poseDataLock.unlock();
}

// ----------------------------------------------------------------------------------------------------