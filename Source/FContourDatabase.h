// ----------------------------------------------------------------------------------------------------
//  Title			FContourDatabase.h
//  Description		Header file for FContourDatabase.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FCONTOURDATABASE_H
#define FCONTOURDATABASE_H

#include "FTrackMe.h"
#include "FVectorT.h"
#include "FMatrix3T.h"
#include "FArchive.h"
#include "FCameraPose.h"
#include "FCameraMetrics.h"
#include "FTrainingParameter.h"
#include "FContourModel.h"
#include "FContourClass.h"
#include "FContourPatch.h"
#include "FContour.h"
#include "FFernTest.h"

// ----------------------------------------------------------------------------------------------------
//  Class FContourDatabase
// ----------------------------------------------------------------------------------------------------

class FContourDatabase
{
	//  Private types ----------------------------------------------------------

private:
	struct hDecomp_t
	{
		FVector3f translation[2];
		FVector3f rotation[2];
		FVector3f normal[2];
	};

	//  Constructors and destructor --------------------------------------------

public:
	/// Default constructor.
	FContourDatabase();
	/// Virtual destructor.
	virtual ~FContourDatabase();

private:
	FContourDatabase(const FContourDatabase& other);
	FContourDatabase& operator=(const FContourDatabase& other);

	//  Public commands --------------------------------------------------------

public:
	/// Resets the database and loads the contour model to be used with this database.
	bool create(const QString& contourFilePath, const QSize& templateSize,
		const QSize& patchSize, const FCameraMetrics& cameraIntrinsics,
		size_t numFerns, size_t numBits);

	/// Checks if the given patch matches any of the already stored template classes.
	/// If a match is found, the class' posterior probability is updated and the function returns false.
	/// Otherwise, a new contour-pose template class is created and true is returned.
	bool insertContourPose(const FContourPatch& contourPatch,
		const FContour* pContour, const FCameraPose& pose);

	/// Sets the squared error threshold that must be exceeded for a new class to be insert.
	void setWarpErrorThreshold(float mseThreshold) { m_warpErrorThreshold = mseThreshold; }
	/// Sets the parameters used for training. This includes the params in serialization.
	void setTrainingParameter(const FTrainingParameter& param) { m_trainingParameter = param; }

	/// Serialization.
	void serialize(FArchive& ar);

	//  Public queries ---------------------------------------------------------

	/// Returns the three most probable classes given the normalized distance map.
	void getBestClassCandidates(const FContourPatch& contourPatch, OUT FContourClass** ppClassList) const;

	/// Returns the class from last insertion that either has been created or matched.
	const FContourClass* lastClass() const { return m_pLastClass; }
	/// Returns the mean squared error from last insertion attempt.
	float lastMeanSquaredError() const { return m_lastMSE; }
	float lastScaledMSE() const { return m_lastScaledMSE; }
	/// Returns the homography from the last insertion attempt.
	const FMatrix3f& lastHomography() const { return m_lastHomography; }


	/// Returns the number of different contour types in the database.
	size_t contourCount() const { return m_data.size(); }
	/// Returns the number of pose classes for the given contour index.
	size_t classCount(size_t index) const { return m_data[index].size(); }
	/// Returns the number of training samples for the given contour index.
	size_t sampleCount(size_t index) const;
	/// Returns the total number of pose classes for all contours.
	size_t totalClassCount() const;
	/// Returns the total number of training samples for all contours.
	size_t totalSampleCount() const;

	/// Returns a measure of fitting accuracy (lower value > more accurate)
	float fittingAccuracy(size_t typeIndex) const;
	/// Returns a measure of pose ambiguity (lower value > more reliable)
	float poseAmbiguity(size_t typeIndex) const;

	/// Returns the contour model the data is based on.
	const FContourModel* contourModel() const { return &m_model; }
	/// Returns the camera metrics used for training of this database.
	const FCameraMetrics& cameraMetrics() const { return m_cameraMetrics; }
	/// Returns the size of templates.
	const QSize& templateSize() const { return m_templateSize; }
	/// Returns the size of warp patches.
	const QSize& patchSize() const { return m_patchSize; }

	/// Returns the squared error threshold that must be exceeded for a new class to be insert.
	float warpErrorThreshold() const { return m_warpErrorThreshold; }
	/// Returns the training parameters.
	const FTrainingParameter& trainingParameter() const { return m_trainingParameter; }

	/// Returns true if the database is initialized and ready for training or classification.
	bool isValid() const { return m_isValid; }

	/// Writes information about the internal state to the given debug object.
	void dump(QDebug& debug) const;

	//  Internal functions -----------------------------------------------------

private:
	/// Deletes all allocated data in the database.
	void _clear();

	/// Fits the contour to each class' distance map. If a match is found, the class is returned.
	FContourClass* _matchClass(const FContour* pContour);

	/// Returns the three most probable classes given the descriptor.
	void _getBestClassCandidates(quint32* pDescriptor, OUT FContourClass** ppClassList) const;

	/// Homography decomposition.
	void _decomposeHomography(const float* pHomography, float fovY, hDecomp_t& decomposition);

	/// Builds the NKP 3x3 transformation matrix from the given camera pose and contour transform.
	void _buildMatrixNKP(const FCameraPose& cameraPose, const FCameraMetrics& metrics,
						 const FContour* pContour, OUT FMatrix3f& matNKP);

	//  Internal data members --------------------------------------------------

private:
	static const quint32 MAX_FERNS = 64;
	static const quint32 MAX_BITS = 32;

	typedef std::list<FContourClass*> classList_t;
	typedef std::vector<classList_t> contourList_t;

	contourList_t m_data;
	FContourModel m_model;
	FFernTest m_test;

	QSize m_templateSize;
	QSize m_patchSize;
	FCameraMetrics m_cameraMetrics;
	quint32 m_numFerns;
	quint32 m_numBits;
	float m_warpErrorThreshold;
	FTrainingParameter m_trainingParameter;

	bool m_isValid;

	// temporary
	FContourClass* m_pLastClass;
	float m_lastMSE;
	float m_lastScaledMSE;
	FMatrix3f m_lastHomography;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FCONTOURDATABASE_H