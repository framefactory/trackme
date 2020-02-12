// ----------------------------------------------------------------------------------------------------
//  Title			FStreamEngine.cpp
//  Description		Implementation of class FStreamEngine
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "levmar.h"
#include "FDetectorThread.h"

#include "FStreamEngine.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamEngine
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamEngine::FStreamEngine(QObject* pParent /* = NULL */)
: QObject(pParent),
  m_pLineTracker(NULL),
  m_pPoseDetector(NULL),
  m_pDetectorThread(NULL),
  m_pDTBuffer(NULL),
  m_detectionEnabled(true),
  m_detectionAlwaysOn(false),
  m_wantRedraw(false),
  m_frameSize(0, 0),
  m_frameIndex(0),
  m_undistFactor(0.0f, 0.0f)
{
	// TODO: Dirty hack!
	FGlobalConstants::pLevmarDetectionWorkspace = new float[LM_DER_WORKSZ(8, 4096)];
	FGlobalConstants::pLevmarTrackingWorkspace = new double[LM_DIF_WORKSZ(7, 4096)];


	m_pLineTracker = new FLineTracker();
	m_pLineTracker->setCamera(&m_camera);
	m_pPoseDetector = new FPoseDetector();

	m_pDetectorThread = new FDetectorThread();
	m_pDetectorThread->setPoseDetector(m_pPoseDetector);
	m_pDetectorThread->start(QThread::NormalPriority);

	m_augmentedPosition.makeZero();
	m_augmentedRotation.makeZero();
	m_augmentedScale = 1.0f;
	m_augmentedTranslation.makeIdentity();
	
	_initGL();
}

FStreamEngine::~FStreamEngine()
{
	m_pDetectorThread->stop();
	F_SAFE_DELETE(m_pDetectorThread);
	F_SAFE_DELETE_ARRAY(m_pDTBuffer);
	F_SAFE_DELETE(m_pLineTracker);
	F_SAFE_DELETE(m_pPoseDetector);

	// TODO: Dirty hack!
	F_SAFE_DELETE_ARRAY(FGlobalConstants::pLevmarDetectionWorkspace);
	F_SAFE_DELETE_ARRAY(FGlobalConstants::pLevmarTrackingWorkspace);
}

// Public commands ------------------------------------------------------------------------------------

void FStreamEngine::reset(const QSize& frameSize)
{
	F_DEBUG("FStreamEngine", QString("Reset - New frame size: %1 x %2")
		.arg(frameSize.width()).arg(frameSize.height()));

	m_frameSize = frameSize;
	m_pLineTracker->reset(frameSize);
	m_pPoseDetector->reset(frameSize);

	F_SAFE_DELETE_ARRAY(m_pDTBuffer);
	m_pDTBuffer = new FDTPixel[frameSize.width() * frameSize.height()];	

	_resetGL();

	m_camera.setImageSize(FVector2f(frameSize.width(), frameSize.height()));
	m_camera.resetPose();
}

void FStreamEngine::processFrame(const FGLTextureRect& inputFrame, FFrameStatistics* pStats /* = NULL */)
{
	m_inputFrame = inputFrame;

	size_t prevIndex = m_frameIndex;
	m_frameIndex = (m_frameIndex + 1) % FRAME_BUFFER_SIZE;

	_radialUndistort();
	glFlush();

	m_pLineTracker->searchCandidates(
		m_texPreprocessed[m_frameIndex], m_texPreprocessed[prevIndex], &pStats->tracker);
	glFlush();

	// nudge detection if detector thread is idle
	if (m_pDetectorThread->isIdle())
	{
		if (pStats)
			pStats->detector = m_pDetectorThread->statistics();

		m_pPoseDetector->getPreprocessingResult(m_pDTBuffer);
		m_pDetectorThread->processFrame(m_pDTBuffer);
	}

	// run preprocessing stage of detector on GPU (canny edges, distance transform)
	m_pPoseDetector->preprocess(inputFrame);
	glFlush();

	// optimize pose on line tracker
	m_pLineTracker->optimizePose();

	if (pStats)
		pStats->tracker.state = m_pLineTracker->state();

	// if tracker fails, use last pose detector result
	if ((m_pLineTracker->state() == FLineTrackerState::Failed && m_detectionEnabled)
		|| m_detectionAlwaysOn)
	{
		//m_pPoseDetector->detect(inputFrame, &pStats->detector);

		// try all pose estimates from the detector
		m_pDetectorThread->lockPoseData();
		size_t n = m_pDetectorThread->poseCount();
		size_t i;
		for (i = 0; i < n; i++)
		{
			m_camera.resetPose(m_pDetectorThread->detectedPose(i));
			m_pLineTracker->searchCandidates(
				m_texPreprocessed[m_frameIndex], m_texPreprocessed[prevIndex], &pStats->tracker);
			m_pLineTracker->optimizePose();
			if (m_pLineTracker->state() == FLineTrackerState::Tracking)
				break;
		}
		m_pDetectorThread->unlockPoseData();

		if (pStats)
		{
			pStats->detector.numPoses = n;
			pStats->detector.poseUsed = i < n ? i : -1;
		}
	}
	else
	{
		if (pStats)
		{
			pStats->detector.numPoses = m_pDetectorThread->poseCount();
			pStats->detector.poseUsed = -1;
		}
	}

	//m_harris.track(inputFrame);

	if (pStats)
	{
		for (size_t i = 0; i < 7; i++)
		{
			pStats->finalPose[i] = m_camera.poseCurrent(i);
			pStats->finalPoseSmooth[i] = m_camera.poseSmooth(i);
		}
	}
}

void FStreamEngine::resetPose()
{
	m_camera.resetPose();
}

void FStreamEngine::drawSolidModel()
{
	if (m_pLineTracker)
		m_pLineTracker->drawSolidModel();
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FStreamEngine::previousFrame() const
{
	return m_texPreprocessed[(m_frameIndex - 1 + FRAME_BUFFER_SIZE) % FRAME_BUFFER_SIZE];
}

const FGLTextureRect& FStreamEngine::frameAt(size_t index) const
{
	F_ASSERT(index < FRAME_BUFFER_SIZE);
	return m_texPreprocessed[(m_frameIndex - index + FRAME_BUFFER_SIZE) % FRAME_BUFFER_SIZE];
}

const FGLTextureRect& FStreamEngine::drawAugmentedImage()
{
	FMatrix4f matPGL, matMV, matMVPGL;
	m_camera.getProjectionGLSmooth(matPGL);
	m_camera.getModelViewSmooth(matMV);
	matMVPGL = matPGL * matMV;
	matMV.transpose();
	matMVPGL.transpose();

	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL.ptr(), matSize);
	m_bufModelTransform.write(matMV.ptr(), matSize, matSize);

	//m_fbAugmentedModel.bind(1);
	FGLFramebuffer::bindDefault();

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	glClearColor(0.0, 0.0, 0.0, 0.0);
	glClear(/* GL_COLOR_BUFFER_BIT | */ GL_DEPTH_BUFFER_BIT);

	// safety: don't draw anything if tracker is not working properly
	if (m_pLineTracker->state() != FLineTrackerState::Tracking)
	{
		FGLFramebuffer::bindDefault();
		return m_texAugmentedModelColor;
	}

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	glColorMaski(0, false, false, false, false);
	
	m_prgModelDepth.bind();
	m_prgModelDepth.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	// draw solid model to depth buffer
	m_pLineTracker->model()->drawSolidGL();

	m_camera.getModelViewSmooth(matMV);
	matMV *= m_augmentedTranslation;
	matMVPGL = matPGL * matMV;
	matMV.transpose();
	matMVPGL.transpose();
	m_bufModelTransform.write(matMVPGL.ptr(), matSize);
	m_bufModelTransform.write(matMV.ptr(), matSize, matSize);


	glColorMaski(0, true, true, true, true);
	
	m_prgModelTextured.bind();
	m_prgModelTextured.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	for (size_t i = 0, n = m_augmentedModel.subsetCount(); i < n; ++i)
	{
		if (m_augmentedModel.textureAt(i).isValid())
		{
			m_augmentedModel.textureAt(i).bind(0);
			glUniform1i(m_uTextureEnabled, 1);
		}
		else
			glUniform1i(m_uTextureEnabled, 0);

		m_augmentedModel.draw(i);
	}
	
	FGLFramebuffer::bindDefault();
	F_GLERROR_ASSERT;

	return m_texAugmentedModelColor;
}

bool FStreamEngine::isPoseDetectorIdle() const
{
	return m_pDetectorThread->isIdle();
}

// Parameter callbacks --------------------------------------------------------------------------------

void FStreamEngine::loadAugmentationModel(QString modelFilePath) {
	m_augmentedModel.import(modelFilePath);
	m_wantRedraw = true;
}

void FStreamEngine::setAugmentedPosition(FVector3d position) {
	m_augmentedPosition.set(position.x(), position.y(), position.z());
	_buildAugmentedMatrixMVP();
	m_wantRedraw = true;
}

void FStreamEngine::setAugmentedRotation(FVector3d rotation) {
	m_augmentedRotation.set(
		FMath::deg2rad(rotation.x()),
		FMath::deg2rad(rotation.y()),
		FMath::deg2rad(rotation.z())
		);
	_buildAugmentedMatrixMVP();
	m_wantRedraw = true;
}

void FStreamEngine::setAugmentedScale(double scale) {
	m_augmentedScale = scale;
	_buildAugmentedMatrixMVP();
	m_wantRedraw = true;
}

void FStreamEngine::loadLineModel(QString modelFilePath) {
	m_pLineTracker->loadModel(modelFilePath);
	m_camera.resetPose();
	m_wantRedraw = true;
}

void FStreamEngine::setCameraOverride(bool state) {
	m_pLineTracker->setTrackingEnabled(!state);
	m_wantRedraw = true;
}

void FStreamEngine::setCameraRadialDistortion(FVector2d factor) {
	m_undistFactor.set(factor.x(), factor.y());
	m_wantRedraw = true;
}

void FStreamEngine::setCameraAperture(FVector2d aperture) {
	m_camera.setAperture(FVector2f(aperture.x(), aperture.y()));
	m_wantRedraw = true;
}

void FStreamEngine::setCameraFocalDistance(double val) {
	m_camera.setFocalDistance(val);
	m_wantRedraw = true;
}

void FStreamEngine::setCameraTranslation(FVector3d translation) {
	FVector3f tr(translation.x(), translation.y(), translation.z());
	m_camera.setTranslation(tr);
	//m_camera.resetPose();
	m_wantRedraw = true;
}

void FStreamEngine::setCameraRotation(FVector3d rotation) {
	FVector3f rot(rotation.x(), rotation.y(), rotation.z());
	m_camera.setRotation(rot);
	//m_camera.resetPose();
	m_wantRedraw = true;
}

void FStreamEngine::setSearchRange(double val) {
	m_pLineTracker->setSearchRange(val);
	m_wantRedraw = true;
}

void FStreamEngine::setSamplingDistance(double val) {
	m_pLineTracker->setSamplingDistance(val);
	m_wantRedraw = true;
}

void FStreamEngine::setSamplingAdaptiveDensity(double val) {
	m_pLineTracker->setSamplingAdaptiveDensity(val);
	m_wantRedraw = true;
}

void FStreamEngine::setMotionCompensation(double val) {
	m_pLineTracker->setMotionCompensation(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerSurfaceAngleLimit(double val) {
	m_pLineTracker->setSurfaceAngleLimit(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerContourAngleLimit(double val) {
	m_pLineTracker->setContourAngleLimit(val);
	m_wantRedraw = true;
}

void FStreamEngine::setEdgeLumaWeight(double val) {
	m_pLineTracker->setEdgeLumaWeight(val);
	m_wantRedraw = true;
}

void FStreamEngine::setEdgeChromaWeight(double val) {
	m_pLineTracker->setEdgeChromaWeight(val);
	m_wantRedraw = true;
}

void FStreamEngine::setEdgeThreshold(double val) {
	m_pLineTracker->setEdgeThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setColorToleranceEnabled(bool state) {
	m_pLineTracker->setColorToleranceEnabled(state);
	m_wantRedraw = true;
}

void FStreamEngine::setColorTolerance(double val) {
	m_pLineTracker->setColorTolerance(val);
	m_wantRedraw = true;
}

void FStreamEngine::setColorAdaptability(double val) {
	m_pLineTracker->setColorAdaptability(val);
	m_wantRedraw = true;
}

void FStreamEngine::setColorPeekDistance(double val) {
	m_pLineTracker->setColorPeekDistance(val);
	m_wantRedraw = true;
}

void FStreamEngine::setColorFullEdgeThreshold(double val) {
	m_pLineTracker->setColorFullEdgeThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setColorHalfEdgeThreshold(double val) {
	m_pLineTracker->setColorHalfEdgeThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setFocalDistanceEnabled(bool val) {
	m_camera.setFocalDistanceEnabled(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerInitializationThreshold(double val) {
	m_pLineTracker->setInitializationThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerFailureThreshold(double val) {
	m_pLineTracker->setFailureThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setMultipleHypothesesEnabled(bool val) {
	m_pLineTracker->setMultipleHypothesesEnabled(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerPredictionFactor(double val) {
	m_pLineTracker->setPredictionFactor(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerInterpolationRate(FVector2d val) {
	m_pLineTracker->setInterpolationRate(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerEstimatorType(int val) {
	m_pLineTracker->setEstimatorType(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerEstimatorLimit(double val) {
	m_pLineTracker->setEstimatorLimit(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerRejectionFactorA(double val) {
	m_pLineTracker->setRejectionFactorA(val);
	m_wantRedraw = true;
}

void FStreamEngine::setTrackerRejectionFactorB(double val) {
	m_pLineTracker->setRejectionFactorB(val);
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionEnabled(bool val) {
	m_detectionEnabled = val;
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionAlwaysOn(bool val) {
	m_detectionAlwaysOn = val;
	m_wantRedraw = true;
}

void FStreamEngine::setEdgeThresholdLow(double val) {
	m_pPoseDetector->setEdgeThresholdLow(val);
	m_wantRedraw = true;
}

void FStreamEngine::setEdgeThresholdHigh(double val) {
	m_pPoseDetector->setEdgeThresholdHigh(val);
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionMSEThreshold(double val) {
	m_pPoseDetector->setMSEThreshold(val);
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionFixedTypeId(int val) {
	m_pPoseDetector->setFixedTypeId(val - 1);
}

void FStreamEngine::setDetectionContourPosition(FVector3d position) {
	m_pPoseDetector->setContourPosition(position);
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionContourRotation(FVector3d rotation) {
	m_pPoseDetector->setContourRotation(rotation);
	m_wantRedraw = true;
}

void FStreamEngine::setDetectionContourScale(double scale) {
	m_pPoseDetector->setContourScale(scale);
	m_wantRedraw = true;
}

void FStreamEngine::loadClassifierDatabase(QString filePath) {
	m_pDetectorThread->stop();
	m_pPoseDetector->loadClassifierData(filePath);
	m_pDetectorThread->start();
	m_wantRedraw = true;
}

// Internal functions ---------------------------------------------------------------------------------

void FStreamEngine::_initGL()
{
	m_prgUndistort.createLinkProgram("Shader/overlay.vert", "Shader/radialUndistort.frag");
	m_uUndistFactor = m_prgUndistort.getUniformLocation("undistFactor");
	m_suSourceFrame = m_prgUndistort.getUniformLocation("sImage");

	m_sampUndistort.create();
	m_sampUndistort.setWrap(FGLWrapMode::Clamp);
	m_sampUndistort.setFilter(FGLFilterType::Linear, FGLFilterType::Linear);

	m_fbPreprocess.create();
	m_augmentedModel.import("Models/teapot.dae");

	m_bufModelTransform.createAllocate(2 * 16 * sizeof(float), FGLUsage::DynamicDraw);
	m_prgModelDepth.createLinkProgram("Shader/solidModel.vert", "Shader/phong2LightsFixed.frag");
	m_prgModelColor.createLinkProgram("Shader/solidModel.vert", "Shader/phong2LightsFixed.frag");
	m_prgModelTextured.createLinkProgram("Shader/solidModel.vert", "Shader/phongTextured.frag");
	m_uTextureEnabled = m_prgModelTextured.getUniformLocation("textureEnabled");
}

void FStreamEngine::_resetGL()
{
	F_ASSERT(!m_frameSize.isEmpty());

	m_overlay.setTexCoords(m_frameSize);
	m_overlay.create();

	for (size_t i = 0; i < FRAME_BUFFER_SIZE; i++)
		m_texPreprocessed[i].createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);

	// Color/depth framebuffer for hidden line rendering
	m_texAugmentedModelColor.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_texAugmentedModelDepth.createAllocate(FGLPixelFormat::D24_UNorm, m_frameSize);
	m_fbAugmentedModel.create();
	m_fbAugmentedModel.attachColorTexture(m_texAugmentedModelColor, 0);
	m_fbAugmentedModel.attachDepthTexture(m_texAugmentedModelDepth);
	F_ASSERT(m_fbAugmentedModel.checkStatus());

}

void FStreamEngine::_radialUndistort()
{
	m_fbPreprocess.attachColorTexture(m_texPreprocessed[m_frameIndex], 0);
	F_ASSERT(m_fbPreprocess.checkStatus());

	m_fbPreprocess.bind(1);
	m_inputFrame.bind(0);
	m_sampUndistort.bind(0);
	m_prgUndistort.bind();
	glUniform2fv(m_uUndistFactor, 1, m_undistFactor.ptr());
	glUniform1i(m_suSourceFrame, 0);

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	m_overlay.draw();
	FGLFramebuffer::bindDefault();
}

void FStreamEngine::_buildAugmentedMatrixMVP()
{
	FMatrix4f matScale;
	matScale.makeScale(m_augmentedScale, m_augmentedScale, m_augmentedScale);
	FMatrix4f matRotate;
	matRotate.makeRotationYPR(m_augmentedRotation);
	m_augmentedTranslation = matRotate * matScale;
	m_augmentedTranslation.setTranslation(m_augmentedPosition);
}

// ----------------------------------------------------------------------------------------------------