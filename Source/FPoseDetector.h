// ----------------------------------------------------------------------------------------------------
//  Title			FPoseDetector.h
//  Description		Header file for FPoseDetector.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-09 13:00:10 +0200 (Fr, 09 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FPOSEDETECTOR_H
#define FPOSEDETECTOR_H

#include "FTrackMe.h"
#include "FlowMath.h"
#include "FlowGL.h"
#include "FStopWatch.h"
#include "FDistanceTransform.h"
#include "FContourFinder.h"
#include "FContourDatabase.h"
#include "FFrameStatistics.h"

// ----------------------------------------------------------------------------------------------------
//  Class FPoseDetector
// ----------------------------------------------------------------------------------------------------

class FPoseDetector
{
	//  Static callbacks -------------------------------------------------------

private:
	inline static void sLevmarUpdate(float* p, float* hx, int m, int n, void* pData) {
		F_ASSERT(pData);
		((FPoseDetector*)pData)->_levmarUpdate(p, hx, m, n);
	}

	//  Private types ----------------------------------------------------------

private:
	struct contourInfo_t
	{
		contourInfo_t() { clear(); typeId = 0; }

		contourInfo_t(size_t _typeId, const FVector3f& _center)
			: typeId(_typeId), center(_center) {
			clear();
		}

		void clear() {
			pContour = NULL; pPatch = NULL; pClass = NULL;
			meanSquareError = reconstructionAngle = FLT_MAX;
		}

		size_t typeId;
		const FContour* pContour;
		FContourPatch* pPatch;
		FContourClass* pClass;

		FVector3f center;
		FMatrix3f homography;
		float meanSquareError;
		float reconstructionAngle;
		FMatrix3f matRot;
		FVector3f translation;
		float alignError;
		float alignAngle;
	};

	// sort predicates
	static bool contourInfoSortByAmbiguity(contourInfo_t* pInfo1, contourInfo_t* pInfo2) {
		return pInfo1->pClass->poseAmbiguity() < pInfo2->pClass->poseAmbiguity();
	}
	static bool contourInfoSortByAccuracy(contourInfo_t* pInfo1, contourInfo_t* pInfo2) {
		return pInfo1->pClass->fittingAccuracy() < pInfo2->pClass->fittingAccuracy();
	}

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FPoseDetector();
	/// Virtual destructor.
	virtual ~FPoseDetector();

	//  Public commands --------------------------------------------------------

	/// Pre-processes the given image on the GPU, i.e. runs canny edge
	/// detection and distance transform.
	void preprocess(const FGLTextureRect& inputImage);

	/// Detects the existence and pose of the current model in the given image.
	/// Returns true if the detection was successful.
	void detect(const FGLTextureRect& inputImage, FDetectorStatistics* pStats = NULL);

	/// Detector start for multi threaded application. Expects a precalculated DT map.
	void detect(FDTPixel* pDTImage, FDetectorStatistics* pStats = NULL);

	/// Detector start for verification of the training database. Expects
	/// a texture with a contour drawing.
	void detectVerify(const FGLTextureRect& contourImage);

	/// Resets the pose detector and sets the frame size of the internal pipeline.
	bool reset(const QSize& frameSize);

	/// Loads classifier data for pose detection.
	bool loadClassifierData(const QString& dataFilePath);
	/// Uses the classifier data from the given database.
	void setClassifierData(FContourDatabase* pDatabase);

	//  PARAMETER

	void setEdgeThresholdLow(float val) { m_edgeThresholdLow = val; }
	void setEdgeThresholdHigh(float val) { m_edgeThresholdHigh = val; }
	void setMSEThreshold(float val) { m_warpErrorThreshold = val; }
	void setFixedTypeId(int val) { m_fixedTypeId = val; }

	void setContourPosition(const FVector3f& pos) {
		m_contourPosition = pos;
		_updateContourRelativePose();
	}
	void setContourRotation(const FVector3f& rot) {
		m_contourRotation = rot;
		_updateContourRelativePose();
	}
	void setContourScale(float scale) {
		m_contourScale = scale;
		_updateContourRelativePose();
	}

	//  Public queries ---------------------------------------------------------

	/// Returns true if the pose detector has been initialized successfully.
	bool isValid() const { return m_isValid; }

	/// Copies the result of the preprocessing step to the given array.
	void getPreprocessingResult(FDTPixel* pData);

	//  RESULTS FROM INTERMEDIATE PROCESSING STEPS

	/// Returns the texture containing the result from canny edge detection.
	const FGLTextureRect& cannyEdges() const;
	/// Returns the texture containing the result from distance transform.
	const FGLTextureRect& distanceTransform() const;
	/// Returns the texture containing the detected contours.
	const FGLTextureRect& contourView();
	/// Draws the contour statistics (mean, variance, bounding box) using the given canvas.
	void drawContourStatistics(FGLCanvas& canvas);
	/// Draws the detected contour patches.
	void drawDetectionMap();
	/// Draws an axis indicating the detected pose.
	void drawDetectedPose();

	/// Returns true if one or more pose candidates could be estimated.
	bool hasValidPose() const { return m_poseCount > 0; }
	/// Returns the number of pose candidates.
	size_t poseCount() const { return m_poseCount; }
	
	/// Returns a pose candidate.
	const FCameraPose& detectedPose(size_t index) {
		F_ASSERT(index < m_poseCount);
		return m_detectedPose[index];
	}

	size_t poseTypeId(size_t index) { return m_detectedPoseInfo[index]->pClass->templateIndex(); }
	float poseClassAmbiguity(size_t index) { return m_detectedPoseInfo[index]->pClass->poseAmbiguity(); }
	float poseClassAccuracy(size_t index) { return m_detectedPoseInfo[index]->pClass->fittingAccuracy(); }
	float poseFittingError(size_t index) { return m_detectedPoseInfo[index]->meanSquareError; }
	float poseReconstructionAngle(size_t index) { return m_detectedPoseInfo[index]->reconstructionAngle; }

	//  Internal functions -----------------------------------------------------

private:
	void _initializeDatabase();
	void _runCanny(const FGLTextureRect& inputImage);
	void _matchContours();
	void _matchContour(const FContour* pContour, size_t candidateIndex);
	float _reconstructPose(contourInfo_t& contour);
	void _levmarUpdate(float* p, float* hx, int m, int n);
	void _decomposeHomography(const FMatrix3f& homography, FMatrix3f& rotation, FVector3f& translation);
	void _changePatchSize(const QSize& patchSize);
	void _updateContourRelativePose();
	bool _initGL();
	bool _resetGL();

	//  Internal data members --------------------------------------------------

	static const size_t MAX_CLASS_CANDIDATES = 3;
	static const size_t MAX_DISPLAY_SLOTS = 16;
	static const size_t MAX_POSE_CANDIDATES = 8;

	bool m_isValid;
	bool m_ownDatabase;
	QSize m_frameSize;
	QSize m_templateSize;
	QSize m_patchSize;
	QSize m_slotSize;

	FDistanceTransform m_distanceTransform;
	FContourFinder m_contourFinder;
	FContourDatabase* m_pClassifierData;
	FCameraMetrics m_cameraMetrics;

	// Statistics
	FDetectorStatistics* m_pStatistics;
	FStopWatch m_stopWatch;

	// Temporary
	FContourPatch m_patch[FGlobalConstants::MAX_CONTOUR_CANDIDATES];
	contourInfo_t m_contour[FGlobalConstants::MAX_TEMPLATES];
	int m_trustMap[FGlobalConstants::MAX_TEMPLATES][FGlobalConstants::MAX_TEMPLATES];
	size_t m_optAlignAnchor;
	size_t m_optAlignSubject;
	size_t m_optActiveContours;

	// Contour pose
	FVector3f m_contourPosition;
	FVector3f m_contourRotation;
	float m_contourScale;
	FMatrix3f m_matContourRot;

	// Final pose
	size_t m_poseCount;
	FCameraPose m_detectedPose[MAX_POSE_CANDIDATES];
	contourInfo_t* m_detectedPoseInfo[MAX_POSE_CANDIDATES];

	// Parameter
	float m_edgeThresholdLow;
	quint32 m_uEdgeThresholdLow;
	float m_edgeThresholdHigh;
	quint32 m_uEdgeThresholdHigh;
	float m_warpErrorThreshold;
	int m_fixedTypeId;

	// OpenGL
	FGLOverlayRect m_overlay;

	FGLFramebuffer m_fbBuffer[2];
	FGLTextureRect m_texBuffer[2];

	FGLProgram m_prgGaussian3x3;
	FGLProgram m_prgGaussianVert;
	FGLProgram m_prgGaussianHorz;
	FGLProgram m_prgBoxBlurVert;
	FGLProgram m_prgBoxBlurHorz;
	FGLProgram m_prgCannyDetect;
	FGLProgram m_prgCannySuppress;
	FGLProgram m_prgCannyReduce1;
	FGLProgram m_prgCannyReduce2;
	FGLProgram m_prgCannyThreshold;

	// detection map drawing
	FGLFramebuffer m_fbDetectionMap;
	FGLTextureRect m_texDetectionMap;
	FGLProgram m_prgDetectionMap;
	FGLTextureRect m_texPatch[MAX_DISPLAY_SLOTS * 2];
	FGLOverlayRect m_overlayPatch[MAX_DISPLAY_SLOTS * 2];

	// pose drawing
	FGLMesh m_axisModel;
	FGLProgram m_prgSolidModel;
	FGLProgram m_prgContourModel;
	FGLBuffer m_bufTransform;
	quint32 m_uTransformBlock1;
	quint32 m_uTransformBlock2;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FPOSEDETECTOR_H