// ----------------------------------------------------------------------------------------------------
//  Title			FDetectorThread.h
//  Description		Header file for FDetectorThread.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/14 $
// ----------------------------------------------------------------------------------------------------

#ifndef FDETECTORTHREAD_H
#define FDETECTORTHREAD_H

#include <QThread>
#include <QMutex>
#include <QWaitCondition>

#include "FTrackMe.h"
#include "FDTPixel.h"
#include "FCameraPose.h"
#include "FFrameStatistics.h"

class FPoseDetector;

// ----------------------------------------------------------------------------------------------------
//  Class FDetectorThread
// ----------------------------------------------------------------------------------------------------

class FDetectorThread : public QThread
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FDetectorThread(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FDetectorThread();

	//  Public commands --------------------------------------------------------

public:
	/// Starts the thread.
	void start(QThread::Priority priority = QThread::NormalPriority);
	/// Stops the thread.
	void stop();

	/// Processes one frame.
	void processFrame(FDTPixel* pDistanceTransform);

	/// Sets the pose detector to be used.
	void setPoseDetector(FPoseDetector* pDetector);

	/// Locks the access to the detected pose data.
	void lockPoseData() { m_poseDataLock.lock(); }
	/// Unlocks the access to the detected pose data.
	void unlockPoseData() { m_poseDataLock.unlock(); }

	//  Public queries ---------------------------------------------------------

	/// Returns true if the detector is ready for a new frame.
	bool isIdle() const { return m_isIdle; }

	/// Returns the number of pose candidates.
	size_t poseCount() const { return m_poseCount; }

	/// Returns a pose candidate.
	const FCameraPose& detectedPose(size_t index) const {
		F_ASSERT(index < m_poseCount);
		return m_detectedPose[index];
	}

	/// Returns the statistics for the last run.
	FDetectorStatistics statistics() const { return m_statistics; }

	//  Overrides --------------------------------------------------------------

protected:
	virtual void run();

	//  Internal functions -----------------------------------------------------

private:
	void _processFrame();

	//  Internal data members --------------------------------------------------

private:
	bool m_wantExit;
	bool m_isIdle;

	QMutex m_objectLock;
	QWaitCondition m_frameAvailable;
	mutable QMutex m_poseDataLock;

	FPoseDetector* m_pDetector;
	FDTPixel* m_pDTImage;

	static const size_t MAX_POSE_CANDIDATES = 8;
	FCameraPose m_detectedPose[MAX_POSE_CANDIDATES];
	size_t m_poseCount;

	FDetectorStatistics m_statistics;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FDETECTORTHREAD_H