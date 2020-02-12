// ----------------------------------------------------------------------------------------------------
//  Title			FLineTracker.cpp
//  Description		Implementation of class FLineTracker
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "levmar.h"
#include "FLevmarTermReason.h"
#include "FGenericModel.h"

#include "FLineTracker.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTracker
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FLineTracker::FLineTracker()
: m_pCamera(NULL),
  m_pModel(NULL),
  m_trackerState(FLineTrackerState::Initializing),
  m_usePrevPose(false),
  m_trackingError(0.0f),
  m_blurFilterSteps(180 * 5),
  m_blurFiterWidth(9),
  m_sigmaParallel(/* 1.2f */ 3.5f),
  m_sigmaOrthogonal(/*0.3f */ 0.5f),
  m_pBlurFilter(NULL),
  m_pSearchResult(NULL),
  m_transferSize(0, 0),
  m_pStatistics(NULL),
  m_bufIndex(0),
  m_pResidualData(NULL),
  m_pColorMemoryData(NULL),
  m_toggleId(0)
{
	_initParameters();
	_calculateBlurFilter(m_sigmaParallel, m_sigmaOrthogonal);
	_initGL();

	m_matMVPGL_Previous.makeIdentity();
	m_matMVPGL_Previous2.makeIdentity();
}

FLineTracker::~FLineTracker()
{
	F_SAFE_DELETE(m_pModel);
	F_SAFE_DELETE_ARRAY(m_pBlurFilter);
	F_SAFE_DELETE_ARRAY(m_pSearchResult);
	F_SAFE_DELETE_ARRAY(m_pResidualData);
	F_SAFE_DELETE_ARRAY(m_pColorMemoryData);
}

// Public commands ------------------------------------------------------------------------------------

void FLineTracker::searchCandidates(const FGLTextureRect& currentFrame, const FGLTextureRect& previousFrame,
						 FTrackerStatistics* pStats /* = NULL */)
{
	if (!m_pModel)
		return;

	m_currentFrame = currentFrame;
	m_previousFrame = previousFrame;

	m_pStatistics = pStats;

	F_ASSERT(m_currentFrame.size() == m_frameSize);
	F_ASSERT(!m_previousFrame.isValid() || m_previousFrame.size() == m_frameSize);
	F_ASSERT(m_pCamera);
	F_ASSERT(m_pModel->isValid());

	// search stage
	if (m_pStatistics)
		m_stopWatch.start();

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_pCamera->advanceFrame(m_motionPredictionFactor);
	if (m_trackerState == FLineTrackerState::Disabled)
		m_pCamera->resetPose();

	_searchCandidates();

	if (m_pStatistics)
	{
		m_pStatistics->timeSearch = m_stopWatch.lapse();
		m_stopWatch.reset();
	}
}

void FLineTracker::optimizePose()
{
	if (!m_pModel)
		return;

	_drawInitialPose(); // for display only

	if (m_trackerState != FLineTrackerState::Disabled)
	{
		if (m_pModel->costVectorSize() < 24)
		{
			m_trackerState = FLineTrackerState::Failed;
			m_trackingError = 0.0f;
			m_usePrevPose = false;
			return;
		}
		else
		{
			if (m_trackerState != FLineTrackerState::Tracking)
			{
				_resetColorStatistics();
				m_usePrevPose = false;
			}

			m_trackingError = _optimizePose();
			
			if (m_trackerState == FLineTrackerState::Tracking)
			{
				if (m_trackingError > m_failureErrorThreshold)
					m_trackerState = FLineTrackerState::Failed;
			}
			else
			{
				if (m_trackingError < m_initializationErrorThreshold)
					m_trackerState = FLineTrackerState::Tracking;
			}

			if (m_trackerState == FLineTrackerState::Tracking)
			{
				_updateColorStatistics();
				m_usePrevPose = true;
			}
		}
	}
}

void FLineTracker::reset(const QSize& frameSize)
{
	F_ASSERT(!frameSize.isEmpty());
	m_frameSize = frameSize;

	fInfo("Line Tracker", QString("Reset - Frame size: %1 x %2")
		.arg(frameSize.width()).arg(frameSize.height()));

	_frameSizeChanged_resetGL();
	m_usePrevPose = false;
}

void FLineTracker::setCamera(FCamera* pCamera)
{
	m_pCamera = pCamera;
}

void FLineTracker::loadModel(const QString& modelFilePath)
{
	FGenericModel* pModel = new FGenericModel();
	pModel->createImportModel(modelFilePath);
	if (!pModel->isValid() || pModel->edgeCount() == 0)
	{
		fWarning("Line Tracker", QString("Failed to load model from: %1").arg(modelFilePath));
		F_SAFE_DELETE(pModel);
		return;
	}

	F_SAFE_DELETE(m_pModel);
	m_pModel = pModel;

	m_pModel->setMethod(m_multiHypothesesEnabled
		? FLineModel::MultipleHypotheses : FLineModel::SingleHypothesis);

	m_transferSize.setWidth(m_pModel->edgeCount() * FGlobalConstants::MAX_SAMPLES_PER_EDGE);
	m_transferSize.setHeight(FGlobalConstants::SEARCH_LINE_LENGTH);

	_modelChanged_resetGL();

	int lastSlash = modelFilePath.lastIndexOf("/");
	fInfo("Line Tracker", QString("Model loaded: %1, Transfer Block: %2 x %3")
		.arg(modelFilePath.mid(lastSlash + 1)).arg(m_transferSize.width()).arg(m_transferSize.height()));
}

void FLineTracker::setTrackingEnabled(bool state)
{
	if (state && m_trackerState == FLineTrackerState::Disabled)
		m_trackerState = FLineTrackerState::Initializing;
	else if (!state)
		m_trackerState = FLineTrackerState::Disabled;
}

void FLineTracker::drawSolidModel()
{
	if (!m_pModel)
		return;

	F_ASSERT(m_pCamera);
	F_ASSERT(m_pModel->isValid());

	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set up model-view-projection transformation for edge model
	FMatrix4f matMV_Start, matPGL_Start, matMVPGL_Start;
	m_pCamera->getModelViewExtra(matMV_Start);
	m_pCamera->getProjectionGLExtra(matPGL_Start);
	matMVPGL_Start = matPGL_Start * matMV_Start;

	matMVPGL_Start.transpose();
	matMV_Start.transpose();

	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL_Start.ptr(), matSize);
	m_bufModelTransform.write(matMV_Start.ptr(), matSize, matSize);

	// Draw solid object
	m_prgModelSolid.bind();
	m_prgModelSolid.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);
	m_pModel->drawSolidGL();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

void FLineTracker::setMultipleHypothesesEnabled(bool state)
{
	m_multiHypothesesEnabled = state;
	if (m_pModel)
		m_pModel->setMethod(state ? FLineModel::MultipleHypotheses : FLineModel::SingleHypothesis);
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FLineTracker::initialModel()
{
	return m_texOverlayInitialPose;
}

const FGLTextureRect& FLineTracker::fittedModel()
{
	F_ASSERT(m_pCamera);

	if (m_pModel && m_pModel->isValid())
		_drawFittedPose();

	return m_texOverlayFittedPose;
}

const FGLTextureRect& FLineTracker::referenceColors()
{
	F_ASSERT(m_pCamera);

	if (m_pModel && m_pModel->isValid())
		_drawReferenceColors();
	
	return m_texOverlayReferenceColors;
}

// Internal functions ---------------------------------------------------------------------------------

void FLineTracker::_initGL()
{
	m_prgModelSolid.createLinkProgram("Shader/solidModel.vert", "Shader/phong2LightsFixed.frag");
	m_prgModelEdges.createLinkProgram("Shader/edgeModel.vert", "Shader/edgePass.frag");
	m_prgModelDepth.createLinkProgram("Shader/meshModel.vert", "Shader/depthPass.frag");
	
	m_prgModelSearchLines.createLinkProgram("Shader/sampleModel.vert",
		"Shader/drawSampleLines.geom", "Shader/drawSampleLines.frag");

	m_suDepthModel2 = m_prgModelSearchLines.getUniformLocation("sDepthModel");
	m_uImageSize2 = m_prgModelSearchLines.getUniformLocation("imageSize");
	m_uSearchRange2 = m_prgModelSearchLines.getUniformLocation("searchRange");
	m_uSamplingDistance2 = m_prgModelSearchLines.getUniformLocation("minSampleDistance");
	m_uSampleAdaptiveDensity2= m_prgModelSearchLines.getUniformLocation("sampleAdaptiveDensity");
	m_uMotionCompensation2= m_prgModelSearchLines.getUniformLocation("motionCompensation");

	m_prgModelSample.createLinkProgram("Shader/sampleModel.vert",
		"Shader/sampleModel_3.geom", "Shader/sampleModel_3.frag");
	
	m_suDepthModel = m_prgModelSample.getUniformLocation("sDepthModel");
	m_suImage = m_prgModelSample.getUniformLocation("sImage");
	m_suPrevImage = m_prgModelSample.getUniformLocation("sPrevImage");
	m_suFilterKernel = m_prgModelSample.getUniformLocation("sFilterKernel");
	m_suColorBuffer1 = m_prgModelSample.getUniformLocation("sColorBuffer1");
	if (FGlobalConstants::USE_ACM_V2)
		m_suColorBuffer2 = m_prgModelSample.getUniformLocation("sColorBuffer2");
	m_suResidualData = m_prgModelSample.getUniformLocation("sResidualData");
	m_uSearchRange = m_prgModelSample.getUniformLocation("searchRange");
	m_uSamplingDistance = m_prgModelSample.getUniformLocation("minSampleDistance");
	m_uSampleAdaptiveDensity = m_prgModelSample.getUniformLocation("sampleAdaptiveDensity");
	m_uMotionCompensation = m_prgModelSample.getUniformLocation("motionCompensation");
	m_uSurfaceAngleLimit = m_prgModelSample.getUniformLocation("surfaceAngleLimit");
	m_uContourAngleLimit = m_prgModelSample.getUniformLocation("contourAngleLimit");

	m_uFilterWidth = m_prgModelSample.getUniformLocation("filterWidth");
	m_uEdgeLumaWeight = m_prgModelSample.getUniformLocation("edgeLumaWeight");
	m_uEdgeChromaWeight = m_prgModelSample.getUniformLocation("edgeChromaWeight");
	m_uEdgeThreshold = m_prgModelSample.getUniformLocation("edgeThreshold");
	m_uColorPeekDistance = m_prgModelSample.getUniformLocation("colorPeekDistance");
	m_uColorFullEdgeThreshold = m_prgModelSample.getUniformLocation("colorFullEdgeThreshold");
	m_uColorHalfEdgeThreshold = m_prgModelSample.getUniformLocation("colorHalfEdgeThreshold");
	m_uColorAdaptability = m_prgModelSample.getUniformLocation("colorAdaptability");
	m_uImageSize = m_prgModelSample.getUniformLocation("imageSize");
	m_uTransferSize = m_prgModelSample.getUniformLocation("transferSize");

	m_samplerLinear.create();
	m_samplerLinear.setFilter(FGLFilterType::Linear, FGLFilterType::Linear);
	m_samplerNearest.create();
	m_samplerNearest.setFilter(FGLFilterType::Nearest, FGLFilterType::Nearest);

	FGLShader shOverlay("Shader/overlay.vert");
	FGLShader shSampleEdgeSuppress("Shader/sampleEdgeSuppress_3.frag");
	m_prgModelEdgeSuppress.createLinkProgram(shOverlay, shSampleEdgeSuppress);

	m_bufModelTransform.createAllocate(3 * 16 * sizeof(float), FGLUsage::DynamicDraw);

	size_t numBytes = m_blurFiterWidth * m_blurFiterWidth * m_blurFilterSteps * sizeof(float);
	m_bufBlurFilter.createInitialize(m_pBlurFilter, numBytes, FGLUsage::StaticDraw);
	m_texBlurFilter.createAttach(m_bufBlurFilter, FGLPixelFormat::R32_Float);

	// Adaptive color matching V2
	m_prgSampleColors.createLinkProgram("shader/sampleModel.vert",
		"shader/sampleColors.geom", "shader/sampleColors.frag");
	
	m_prgSampleColors.bindAttribLocation(0, "vecPosition");
	m_prgSampleColors.bindAttribLocation(1, "vecNormal");
	m_prgSampleColors.bindAttribLocation(2, "sampleDensity");

	m_uUC_ImageSize = m_prgSampleColors.getUniformLocation("imageSize");
	m_uUC_TransferSize = m_prgSampleColors.getUniformLocation("transferSize");
	m_uUC_MinSampleDistance = m_prgSampleColors.getUniformLocation("minSampleDistance");
	m_uUC_SampleAdaptiveDensity = m_prgSampleColors.getUniformLocation("sampleAdaptiveDensity");
	m_uUC_SearchRange = m_prgSampleColors.getUniformLocation("searchRange");
	m_uUC_ColorPeekDistance = m_prgSampleColors.getUniformLocation("colorPeekDistance");
	m_uUC_SurfaceAngleLimit = m_prgSampleColors.getUniformLocation("surfaceAngleLimit");
	m_uUC_ContourAngleLimit = m_prgSampleColors.getUniformLocation("contourAngleLimit");
	m_uUC_FilterWidth = m_prgSampleColors.getUniformLocation("filterWidth");
	m_suUC_DepthModel = m_prgSampleColors.getUniformLocation("sDepthModel");
	m_suUC_Image = m_prgSampleColors.getUniformLocation("sImage");
	m_suUC_FilterKernel = m_prgSampleColors.getUniformLocation("sFilterKernel");
	m_suUC_ResidualData = m_prgSampleColors.getUniformLocation("sResidualData");

	m_prgUpdateColors.createLinkProgram("shader/overlay.vert", "shader/updateColors.frag");

	m_prgUpdateColors.bindFragDataLocation(0, "vVertexPosition");
	m_prgUpdateColors.bindFragDataLocation(1, "vVertexTexCoord");

	m_prgUpdateColors.bindFragDataLocation(0, "vOutColor0");
	m_prgUpdateColors.bindFragDataLocation(1, "vOutColor1");
	m_prgUpdateColors.bindFragDataLocation(2, "vOutColor2");
	m_prgUpdateColors.bindFragDataLocation(3, "vOutColor3");

	m_uUC_UpdateColorParams = m_prgUpdateColors.getUniformLocation("updateColorParams");
	m_suUC_SampledColors = m_prgUpdateColors.getUniformLocation("sSampledColors");
	m_suUC_ColorMemory0 = m_prgUpdateColors.getUniformLocation("sColorMemory0");
	m_suUC_ColorMemory1 = m_prgUpdateColors.getUniformLocation("sColorMemory1");
	m_suUC_ColorMemory2 = m_prgUpdateColors.getUniformLocation("sColorMemory2");
	m_suUC_ColorMemory3 = m_prgUpdateColors.getUniformLocation("sColorMemory3");
}

void FLineTracker::_frameSizeChanged_resetGL()
{
	m_overlayRect.setTexCoords(m_frameSize);
	m_overlayRect.create();

	// Color/depth framebuffer for hidden line rendering
	m_fbTexModelColor.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbTexModelDepth.createAllocate(FGLPixelFormat::D24_UNorm, m_frameSize);
	m_fbModelDepthColor.create();
	m_fbModelDepthColor.attachColorTexture(m_fbTexModelColor, 0);
	m_fbModelDepthColor.attachDepthTexture(m_fbTexModelDepth);
	F_ASSERT(m_fbModelDepthColor.checkStatus());

	// Create framebuffer for drawing the initial line model
	m_texOverlayInitialPose.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbOverlayInitialPose.create();
	m_fbOverlayInitialPose.attachColorTexture(m_texOverlayInitialPose, 0);
	F_ASSERT(m_fbOverlayInitialPose.checkStatus());

	// Create framebuffer and canvas for drawing the fitted pose
	m_texOverlayFittedPose.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbOverlayFittedPose.create();
	m_fbOverlayFittedPose.attachColorTexture(m_texOverlayFittedPose, 0);
	F_ASSERT(m_fbOverlayFittedPose.checkStatus());
	m_canvasFittedPose.setCanvasRect(FRect2f(0.0f, 0.0f, m_frameSize.width(), m_frameSize.height()));

	// create framebuffer for drawing the reference colors
	m_texOverlayReferenceColors.createAllocate(FGLPixelFormat::R8G8B8A8_UNorm, m_frameSize);
	m_fbOverlayReferenceColors.create();
	m_fbOverlayReferenceColors.attachColorTexture(m_texOverlayReferenceColors, 0);
	F_ASSERT(m_fbOverlayReferenceColors.checkStatus());
}

void FLineTracker::_modelChanged_resetGL()
{
	m_transferRect.setTexCoords(m_transferSize);
	m_transferRect.create();

	// color frame buffers for storing search results
	m_fbTexModelSearchIntermediate[0].createAllocate(FGLPixelFormat::R32G32B32A32_Float, m_transferSize);
	m_fbTexModelSearchIntermediate[1].createAllocate(FGLPixelFormat::R32G32B32A32_Float, m_transferSize);
	m_fbModelSearchIntermediate.create();

	m_fbTexModelSearchResult.createAllocate(FGLPixelFormat::R32G32B32A32_Float, m_transferSize);
	m_fbModelSearchResult.create();
	m_fbModelSearchResult.attachColorTexture(m_fbTexModelSearchResult, 0);
	F_ASSERT(m_fbModelSearchResult.checkStatus());

	// buffer for confidence (residual)
	F_SAFE_DELETE_ARRAY(m_pResidualData);
	size_t numBytes = m_transferSize.width() * sizeof(float);
	m_pResidualData = new float[numBytes];
	memset(m_pResidualData, 0, numBytes);
	m_bufResidualData.createAllocate(numBytes, FGLUsage::DynamicDraw);
	m_texResidualData.createAttach(m_bufResidualData, FGLPixelFormat::R32_Float);

	// Host memory for search results
	F_SAFE_DELETE_ARRAY(m_pSearchResult);
	m_pSearchResult = new FPixelRGBA32f[m_transferSize.width() * m_transferSize.height()];

	// Clear first intermediate/color buffer
	m_fbModelSearchIntermediate.attachColorTexture(m_fbTexModelSearchIntermediate[0], 0);
	F_ASSERT(m_fbModelSearchIntermediate.checkStatus());
	m_fbModelSearchIntermediate.bind(1);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Clear second intermediate/color buffer
	m_fbModelSearchIntermediate.attachColorTexture(m_fbTexModelSearchIntermediate[1], 0);
	F_ASSERT(m_fbModelSearchIntermediate.checkStatus());
	m_fbModelSearchIntermediate.bind(1);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// ADAPTIVE COLOR MATCHING V2

	QSize colorBufferSize(m_transferSize.width(), 2);
	m_colorMemoryRect.setTexCoords(m_transferSize);
	m_colorMemoryRect.create();

	m_fbColorBuffer.create();
	m_texColorBuffer.createAllocate(FGLPixelFormat::R32G32B32A32_Float, colorBufferSize);
	m_fbColorBuffer.attachColorTexture(m_texColorBuffer, 0);
	F_ASSERT(m_fbColorBuffer.checkStatus());

	// create two frame buffers witch NUM_COLOR_BUFFERS textures attached
	for (int j = 0; j < 2; j++)
	{
		m_fbColorMemory[j].create();
		for (int i = 0; i < NUM_COLOR_BUFFERS; i++)
		{
			m_texColorMemory[i + j * NUM_COLOR_BUFFERS]
				.createAllocate(FGLPixelFormat::R32G32B32A32_Float, colorBufferSize);
			m_fbColorMemory[j].attachColorTexture(m_texColorMemory[i + j * NUM_COLOR_BUFFERS], i);
		}
		F_ASSERT(m_fbColorMemory[j].checkStatus());
	}
	m_toggleId = 0;
	_resetColorStatistics();

	// Host memory for monitoring color memory
	F_SAFE_DELETE_ARRAY(m_pColorMemoryData);
	m_pColorMemoryData = new FPixelRGBA32f[m_transferSize.width() * 2];
	ZeroMemory(m_pColorMemoryData, m_transferSize.width() * 2 * 4 * sizeof(float));


	FGLFramebuffer::bindDefault();
}

void FLineTracker::_initParameters()
{
	m_searchRange = 25.0f;
	m_samplingDistance = 10.0f;
	m_sampleAdaptiveDensity = 0.0f;
	m_surfaceAngleLimit = 3.0f;
	m_contourAngleLimit = 1.0f;

	m_edgeLumaWeight = 1.0f;
	m_edgeChromaWeight = 1.0f;
	m_edgeThreshold = 0.5f;

	m_colorToleranceEnabled = true;
	m_colorTolerance = 0.15f;
	m_colorAdaptability = 0.5f;
	m_colorPeekDistance = 3.0f;
	m_colorFullEdgeThreshold = 3.0f;
	m_colorHalfEdgeThreshold = 1.5f;

	m_motionCompensation = 0.0f;
	
	m_initializationErrorThreshold = 1.5f;
	m_failureErrorThreshold = 3.5f;

	m_multiHypothesesEnabled = true;
	m_interpolationRate.set(2.0f, 6.0f);
	m_motionPredictionFactor = 0.9f;
	m_estimatorType = 0;
	m_estimatorLimit = 8.0f;
	m_rejectionFactorA = 3.0f;
	m_rejectionFactorB = 1.0f;
}

void FLineTracker::_searchCandidates()
{
	// -------- DRAW SOLID MODEL TO Z-BUFFER --------

	// Set up model-view-projection transformation for depth + edge model
	FMatrix4f matMV_Start, matPGL_Start, matMVPGL_Start;
	m_pCamera->getModelViewStart(matMV_Start);
	m_pCamera->getProjectionGLStart(matPGL_Start);
	matMVPGL_Start = matPGL_Start * matMV_Start;
	matMV_Start.transpose();
	matMVPGL_Start.transpose();

	if (!m_usePrevPose)
		m_matMVPGL_Previous = matMVPGL_Start;

	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL_Start.ptr(), matSize, 0);
	m_bufModelTransform.write(matMV_Start.ptr(), matSize, matSize);
	m_bufModelTransform.write(m_matMVPGL_Previous.ptr(), matSize, matSize * 2);

	m_matMVPGL_Previous = matMVPGL_Start;

	// Use color-depth framebuffer to render hidden lines of model
	m_fbModelDepthColor.bind(1);

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw solid object to depth buffer
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glColorMaski(0, false, false, false, false);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	m_prgModelDepth.bind();
	m_prgModelDepth.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);
	m_pModel->drawSolidGL();

	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glColorMaski(0, true, true, true, true);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// -------- DRAW EDGE MODEL / EMIT SEARCH LINES --------

	// Update buffer of residuals (for calculation of color confidence)
	size_t count = m_transferSize.width();
	for (size_t i = 0; i < count; i++)
		m_pResidualData[i] = 0.0f;
	m_pModel->fillResidualData(m_pResidualData, m_colorPeekDistance);
	m_bufResidualData.write(m_pResidualData, count * sizeof(float));

	// Use search target framebuffer
	m_fbModelSearchIntermediate.attachColorTexture(m_fbTexModelSearchIntermediate[m_bufIndex], 0);
	F_ASSERT(m_fbModelSearchIntermediate.checkStatus());
	m_fbModelSearchIntermediate.bind(1);

	glViewport(0, 0, m_transferSize.width(), m_transferSize.height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	// Shader setup
	m_prgModelSample.bind();
	glUniform2f(m_uImageSize, m_frameSize.width(), m_frameSize.height());
	glUniform2f(m_uTransferSize, m_transferSize.width(), m_transferSize.height());
	glUniform1f(m_uSamplingDistance, m_samplingDistance);
	glUniform1f(m_uSearchRange, m_searchRange);
	glUniform1f(m_uSampleAdaptiveDensity, m_sampleAdaptiveDensity);
	glUniform1f(m_uMotionCompensation, m_motionCompensation);
	glUniform1f(m_uSurfaceAngleLimit, FMath::deg2rad(m_surfaceAngleLimit));
	glUniform1f(m_uContourAngleLimit, FMath::deg2rad(m_contourAngleLimit));

	m_prgModelSample.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	// Fragment shader setup
	glUniform1i(m_uFilterWidth, m_blurFiterWidth);
	glUniform1f(m_uEdgeLumaWeight, m_edgeLumaWeight);
	glUniform1f(m_uEdgeChromaWeight, m_edgeChromaWeight);
	glUniform1f(m_uEdgeThreshold, m_edgeThreshold);
	glUniform1f(m_uColorPeekDistance, m_colorPeekDistance);
	glUniform1f(m_uColorFullEdgeThreshold, m_colorFullEdgeThreshold);
	glUniform1f(m_uColorHalfEdgeThreshold, m_colorHalfEdgeThreshold);
	glUniform1f(m_uColorAdaptability, m_colorAdaptability);

	glUniform1i(m_suDepthModel, 0);
	m_fbTexModelDepth.bind(0); 
	m_samplerNearest.bind(0);

	glUniform1i(m_suImage, 1);
	m_currentFrame.bind(1);
	m_samplerLinear.bind(1);

	if (m_previousFrame.isValid())
	{
		glUniform1i(m_suPrevImage, 2);
		m_previousFrame.bind(2);
		m_samplerLinear.bind(2);
	}
	else
	{
		glUniform1i(m_suPrevImage, 1);
	}

	glUniform1i(m_suFilterKernel, 3);
	m_texBlurFilter.bind(3);

	glUniform1i(m_suColorBuffer1, 4);
	m_fbTexModelSearchIntermediate[1 - m_bufIndex].bind(4);
	glUniform1i(m_suResidualData, 5);
	m_texResidualData.bind(5);

	if (FGlobalConstants::USE_ACM_V2)
	{
		glUniform1i(m_suColorBuffer2, 6);
		m_fbColorMemory[m_toggleId * NUM_COLOR_BUFFERS].bind(6);
	}

	m_pModel->drawLinesGL();


	// -------- NON-MAXIMUM-SUPPRESSION --------


	// Refine found edges: non-maximum suppression
	m_fbModelSearchResult.bind(1);
	m_prgModelEdgeSuppress.bind();
	m_fbTexModelSearchIntermediate[m_bufIndex].bind(0);
	m_transferRect.draw();

	for (int i = 0; i < 3; i++)
		FGLSampler::unbind(i);

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	FGLFramebuffer::bindDefault(); // bind default target
	F_GLERROR_ASSERT;

	
	// -------- COPY RESULTS TO HOST, GATHER CANDIDATES --------


	// Swap intermediate/color buffer
	m_bufIndex = 1 - m_bufIndex;

	// Retrieve edge candidates from rendered image
	F_ASSERT(m_pSearchResult);
	m_fbTexModelSearchResult.read(FGLDataFormat::RGBA, FGLDataType::Float, m_pSearchResult);

	bool colTolEnabled = m_colorToleranceEnabled;
	int tx = m_transferSize.width();
	int ty = m_transferSize.height();
	int lastLine = tx * (ty - 1);

	m_pModel->beginAddCandidates();

	for (int x = 0; x < tx; x++)
	{
		FPixelRGBA32f& refColor0 = m_pSearchResult[x];
		FPixelRGBA32f& refColor1 = m_pSearchResult[lastLine + x];
		bool firstCandidate = true;

		int edgeId = x / FGlobalConstants::MAX_SAMPLES_PER_EDGE;
		int sampleId = x % FGlobalConstants::MAX_SAMPLES_PER_EDGE;

		for (int y = 1; y < ty - 1; y++)
		{
			FPixelRGBA32f& pixel = m_pSearchResult[y * tx + x];
			if (pixel.r != 0.0f) // edge candidate found
			{
				bool colorOk = (pixel.r > 0.0f);
				float mag = colorOk ? pixel.r : -pixel.r;

				m_pModel->addCandidate(edgeId, sampleId, FVector2f(pixel.b, pixel.a),
					mag, colTolEnabled ? (colorOk ? 1.0f : 0.0f) : 1.0f);

				if (firstCandidate && !FGlobalConstants::USE_ACM_V2)
				{
					m_pModel->addSampleColors(edgeId, sampleId, refColor0, refColor1);
					firstCandidate = false;
				}
			}
		}
	}

	m_pModel->endAddCandidates();
}

float FLineTracker::_optimizePose()
{
	double pCovar[49];

	if (m_pStatistics)
		m_stopWatch.start();

	FMatrix4f matMV_Start, matP_Start, matMVP_Start;
	m_pCamera->getModelViewStart(matMV_Start);
	m_pCamera->getProjectionStart(matP_Start);
	matMVP_Start = matP_Start * matMV_Start;
	m_pModel->transform(matMV_Start, matMVP_Start);
	m_pModel->calculateHypothesis();
	double startCostMedian, startCostMean, startCostSD;
	m_pModel->getCost(startCostMedian, startCostMean, startCostSD);

	FMatrix4f matMV_Extra, matP_Extra, matMVP_Extra;
	m_pCamera->getModelViewExtra(matMV_Extra);
	m_pCamera->getProjectionExtra(matP_Extra);
	matMVP_Extra = matP_Extra * matMV_Extra;
	m_pModel->transform(matMV_Extra, matMVP_Extra);
	m_pModel->calculateHypothesis();

	double extraCostMedian, extraCostMean, extraCostSD;
	m_pModel->getCost(extraCostMedian, extraCostMean, extraCostSD);

	size_t dataCountA = m_pModel->costVectorSize();

	//F_TRACE(QString("INIT, #data: %1, start cost: %2, extrapolated cost: %3")
	//	.arg(dataCountA).arg(startCostMean).arg(extraCostMean));

	m_pModel->markOutliers(extraCostMean + extraCostSD * m_rejectionFactorA);

	size_t dataCountB = m_pModel->costVectorSize();
	//F_TRACE(QString("OUTLIERS REMOVED: %1, remaining %2")
	//	.arg(dataCountA - dataCountB).arg(dataCountB));

	double poseParams[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	int numParameter = m_pCamera->focalDistanceEnabled() ? 7 : 6;
	double opts[] = { 1e-3, 1e-17, 1e-17, 1e-17, 1e-3 }; // tau, eps1, eps2, eps3, delta
	double lmInfo[LM_INFO_SZ];
	double* pBuffer = FGlobalConstants::pLevmarTrackingWorkspace;

	//int method = 2; double c = extraCostAverage + 4.0 * extraCostSD;
	int method = m_estimatorType; double c = m_estimatorLimit;

	int iter = dlevmar_dif(FLineTracker::sLevmarUpdate,
		poseParams, NULL, numParameter, dataCountB, 50, opts, lmInfo, pBuffer, pCovar, (void*)this, method, c);

	double optACostMedian, optACostMean, optACostSD, optBCostMedian, optBCostMean, optBCostSD;
	optBCostMedian = optBCostMean = optBCostSD = 0.0;
	m_pModel->getCost(optACostMedian, optACostMean, optACostSD);
	//F_TRACE(QString("OPT A, avg: %1, sd: %2, #it: %3, %4")
	//	.arg(optACostMean).arg(optACostSD).arg(iter).arg(FLevmarTermReason((int)lmInfo[6]).toString()));

	if (m_pStatistics)
	{
		m_pStatistics->timeOptimizationA = m_stopWatch.stop();
		m_stopWatch.reset();
		m_stopWatch.start();
	}

	m_pModel->markOutliers(optACostMean + optACostSD * m_rejectionFactorB);
	size_t dataCountC = m_pModel->costVectorSize();
	if (dataCountC < dataCountB)
	{
		//F_TRACE(QString("OUTLIERS REMOVED: %1, remaining %2")
		//	.arg(dataCountB - dataCountC).arg(dataCountC));

		double poseParams[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };

		int iter = dlevmar_dif(FLineTracker::sLevmarUpdate,
			poseParams, NULL, numParameter, dataCountC, 50, opts, lmInfo, pBuffer, pCovar, (void*)this, method, c);

		m_pModel->getCost(optBCostMedian, optBCostMean, optBCostSD);
		
		//F_TRACE(QString("OPT B, avg: %1, sd: %2, #it: %3, %4")
		//	.arg(optBCostMean).arg(optBCostSD).arg(iter).arg(FLevmarTermReason((int)lmInfo[6]).toString()));
		//F_TRACE("");
	}
	
	double smoothFactor = FMath::limitedLerp((float)optBCostMean,
		m_interpolationRate.x(), m_interpolationRate.y(), 0.0f, 1.0f);
	m_pCamera->smoothResult(smoothFactor);

	if (m_pStatistics)
	{
		m_pStatistics->timeOptimizationB = m_stopWatch.stop();
		m_stopWatch.reset();

		m_pStatistics->errorMedian[FTrackerStatistics::Start] = startCostMedian;
		m_pStatistics->errorMean[FTrackerStatistics::Start] = startCostMean;
		m_pStatistics->errorSD[FTrackerStatistics::Start] = startCostSD;

		m_pStatistics->errorMedian[FTrackerStatistics::Prediction] = extraCostMedian;
		m_pStatistics->errorMean[FTrackerStatistics::Prediction] = extraCostMean;
		m_pStatistics->errorSD[FTrackerStatistics::Prediction] = extraCostSD;

		m_pStatistics->errorMedian[FTrackerStatistics::OptimizationA] = optACostMedian;
		m_pStatistics->errorMean[FTrackerStatistics::OptimizationA] = optACostMean;
		m_pStatistics->errorSD[FTrackerStatistics::OptimizationA] = optACostSD;

		m_pStatistics->errorMedian[FTrackerStatistics::OptimizationB] = optBCostMedian;
		m_pStatistics->errorMean[FTrackerStatistics::OptimizationB] = optBCostMean;
		m_pStatistics->errorSD[FTrackerStatistics::OptimizationB] = optBCostSD;

		if (pCovar[0] < 100.0)
		{
			m_pStatistics->variance[6] = 0.0;
			for (int i = 0; i < numParameter; i++)
				m_pStatistics->variance[i] = pCovar[i * (numParameter + 1)];
		}
	}

	return optBCostMean > 0.0 ? optBCostMean : optACostMean;
}

void FLineTracker::_updateColorStatistics()
{
	// Uses the final pose after the optimization has been run.
	// First draws the solid model to the z-buffer again to have
	// a depth texture for hidden line removal.

	// Then renders the line model but instead of emitting search
	// lines, colors are sampled on both side of line and the
	// color statistics per sample point is updated.

	// -------- DRAW SOLID MODEL TO Z-BUFFER --------

	// Set up model-view-projection transformation for depth + edge model
	FMatrix4f matMV_Current, matPGL_Current, matMVPGL_Current;
	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionGLCurrent(matPGL_Current);
	matMVPGL_Current = matPGL_Current * matMV_Current;
	matMV_Current.transpose();
	matMVPGL_Current.transpose();

	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL_Current.ptr(), matSize, 0);
	m_bufModelTransform.write(matMV_Current.ptr(), matSize, matSize);

	// Use color-depth framebuffer to render hidden lines of model
	m_fbModelDepthColor.bind(1);

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw solid object to depth buffer
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glColorMaski(0, false, false, false, false);
	glPolygonOffset(1.0f, 1.0f);
	glEnable(GL_POLYGON_OFFSET_FILL);

	m_prgModelDepth.bind();
	m_prgModelDepth.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);
	m_pModel->drawSolidGL();

	glPolygonOffset(0.0f, 0.0f);
	glDisable(GL_POLYGON_OFFSET_FILL);
	glColorMaski(0, true, true, true, true);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);

	// -------- DRAW EDGE MODEL / EMIT SEARCH LINES --------

	m_fbColorBuffer.bind(1);

	// set viewport to size of color memory buffers
	glViewport(0, 0, m_transferSize.width(), 2);
	glClear(GL_COLOR_BUFFER_BIT);

	m_prgSampleColors.bind();
	m_prgSampleColors.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	glUniform2f(m_uUC_ImageSize, m_frameSize.width(), m_frameSize.height());
	glUniform2f(m_uUC_TransferSize, m_transferSize.width(), m_transferSize.height());
	glUniform1f(m_uUC_MinSampleDistance, m_samplingDistance);
	glUniform1f(m_uUC_SearchRange, m_searchRange);
	glUniform1f(m_uUC_SampleAdaptiveDensity, m_sampleAdaptiveDensity);
	glUniform1f(m_uUC_ColorPeekDistance, m_colorPeekDistance);
	glUniform1f(m_uUC_SurfaceAngleLimit, FMath::deg2rad(m_surfaceAngleLimit));
	glUniform1f(m_uUC_ContourAngleLimit, FMath::deg2rad(m_contourAngleLimit));
	glUniform1i(m_uUC_FilterWidth, m_blurFiterWidth);

	glUniform1i(m_suUC_Image, 0);
	glUniform1i(m_suUC_DepthModel, 1);
	glUniform1i(m_suUC_FilterKernel, 2);
	glUniform1i(m_suUC_ResidualData, 3);

	m_currentFrame.bind(0);
	m_samplerLinear.bind(0);
	m_fbTexModelDepth.bind(1); 
	m_samplerNearest.bind(1);
	m_texBlurFilter.bind(2);
	m_texResidualData.bind(3);

	m_pModel->drawLinesGL();


	// -------- UPDATE COLOR STATISTICS --------

	m_fbColorMemory[1 - m_toggleId].bind(4);
	m_prgUpdateColors.bind();

	glUniform2f(m_uUC_UpdateColorParams, m_colorTolerance, m_colorAdaptability);
	glUniform1i(m_suUC_SampledColors, 0);
	glUniform1i(m_suUC_ColorMemory0, 1);
	glUniform1i(m_suUC_ColorMemory1, 2);
	glUniform1i(m_suUC_ColorMemory2, 3);
	glUniform1i(m_suUC_ColorMemory3, 4);

	m_texColorBuffer.bind(0);
	m_texColorMemory[m_toggleId * NUM_COLOR_BUFFERS    ].bind(1);
	m_texColorMemory[m_toggleId * NUM_COLOR_BUFFERS + 1].bind(2);
	m_texColorMemory[m_toggleId * NUM_COLOR_BUFFERS + 2].bind(3);
	m_texColorMemory[m_toggleId * NUM_COLOR_BUFFERS + 3].bind(4);

	m_colorMemoryRect.draw();

	for (int i = 0; i < 8; i++)
		FGLSampler::unbind(i);

	m_toggleId = 1 - m_toggleId;
	FGLFramebuffer::bindDefault();

	// -------- RETRIEVE FIRST COLOR MEMORY --------

	if (FGlobalConstants::USE_ACM_V2)
	{
		m_texColorMemory[m_toggleId * NUM_COLOR_BUFFERS].read(
			FGLDataFormat::RGBA, FGLDataType::Float, m_pColorMemoryData);

		size_t width = m_transferSize.width();
		for (size_t x = 0; x < width; x++)
		{
			size_t edgeId = x / FGlobalConstants::MAX_SAMPLES_PER_EDGE;
			size_t sampleId = x % FGlobalConstants::MAX_SAMPLES_PER_EDGE;
			m_pModel->addSampleColors(edgeId, sampleId,
				m_pColorMemoryData[x + width], m_pColorMemoryData[x]);
		}
	}
}

void FLineTracker::_resetColorStatistics()
{
	for (int j = 0; j < 2; j++)
	{
		// initialize color memory textures
		m_fbColorMemory[j].bind(4);
		glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
		glClear(GL_COLOR_BUFFER_BIT);
	}
}

void FLineTracker::_levmarUpdate(double* pPoseParams, double* hx, int m, int n)
{
	F_ASSERT(n == m_pModel->costVectorSize());

	FMatrix4f matMV_Current, matP_Current, matMVP_Current;

	m_pCamera->updatePose(pPoseParams);

	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionCurrent(matP_Current);
	matMVP_Current = matP_Current * matMV_Current;

	m_pModel->transform(matMV_Current, matMVP_Current);
	m_pModel->calculateHypothesis();
	m_pModel->getCostVector(hx);
}

void FLineTracker::_levmarJacobian(double* pParams, double* jac, int m, int n)
{
	F_ASSERT(!"Jacobian not implemented correctly yet");
}

void FLineTracker::_drawInitialPose()
{
	m_fbOverlayInitialPose.bind();
	
	// Draw line model with base pose from previous frame
	FMatrix4f matMV_Start, matPGL_Start, matMVPGL_Start;
	m_pCamera->getModelViewStart(matMV_Start);
	m_pCamera->getProjectionGLStart(matPGL_Start);
	matMVPGL_Start = matPGL_Start * matMV_Start;

	matMV_Start.transpose();
	matMVPGL_Start.transpose();

	if (!m_usePrevPose)
		m_matMVPGL_Previous2 = matMVPGL_Start;


	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL_Start.ptr(), matSize, 0);
	m_bufModelTransform.write(matMV_Start.ptr(), matSize, matSize);
	m_bufModelTransform.write(m_matMVPGL_Previous2.ptr(), matSize, matSize * 2);

	m_matMVPGL_Previous2 = matMVPGL_Start;


	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_prgModelEdges.bind();
	m_prgModelEdges.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	m_pModel->drawLinesGL();

	// Draw search lines
	m_prgModelSearchLines.bind();
	m_prgModelSearchLines.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	glUniform2f(m_uImageSize2, m_frameSize.width(), m_frameSize.height());
	glUniform1f(m_uSamplingDistance2, m_samplingDistance);
	glUniform1f(m_uSearchRange2, m_searchRange);
	glUniform1f(m_uSampleAdaptiveDensity2, m_sampleAdaptiveDensity);
	glUniform1f(m_uMotionCompensation2, m_motionCompensation);
	
	glUniform1i(m_suDepthModel2, 0);
	m_fbTexModelDepth.bind(0); 

	m_pModel->drawLinesGL();



	// Draw line model with predicted (extrapolated) pose
	/*
	FMatrix4f matMV_Extra, matPGL_Extra, matMVPGL_Extra;
	m_pCamera->getModelViewExtra(matMV_Extra);
	m_pCamera->getProjectionGLExtra(matPGL_Extra);
	matMVPGL_Extra = matPGL_Extra * matMV_Extra;
	matMVPGL_Extra.transpose();
	m_bufModelTransform.write(matMVPGL_Extra.ptr(), matSize);
	m_pModel->drawLinesGL();
	*/

	FGLFramebuffer::bindDefault();
}

void FLineTracker::_drawFittedPose()
{
	FMatrix4f matMV_Current, matP_Current, matMVP_Current;
	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionCurrent(matP_Current);
	matMVP_Current = matP_Current * matMV_Current;
	m_pModel->transform(matMV_Current, matMVP_Current);

	m_fbOverlayFittedPose.bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_canvasFittedPose.clear();
	m_pModel->drawCandidates(m_canvasFittedPose);
	m_canvasFittedPose.draw();

	FGLFramebuffer::bindDefault();
}

void FLineTracker::_drawReferenceColors()
{
	FMatrix4f matMV_Current, matP_Current, matMVP_Current;
	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionCurrent(matP_Current);
	matMVP_Current = matP_Current * matMV_Current;
	m_pModel->transform(matMV_Current, matMVP_Current);

	m_fbOverlayReferenceColors.bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_canvasFittedPose.clear();
	m_pModel->drawReferenceColors(m_canvasFittedPose);
	m_canvasFittedPose.draw();

	FGLFramebuffer::bindDefault();
}

void FLineTracker::_calculateBlurFilter(float sigmaP, float sigmaO)
{
	size_t filterSize = m_blurFiterWidth * m_blurFiterWidth;
	size_t numElements = m_blurFilterSteps * filterSize;

	F_SAFE_DELETE_ARRAY(m_pBlurFilter);
	m_pBlurFilter = new float[numElements];

	float offset = (float)(m_blurFiterWidth / 2);

	for (int f = 0; f < 5; f++)
	{
		float factor = f / 4.0f;
		sigmaO = factor * sigmaP + (1.0f - factor) * sigmaO;

		float sigmaP2i = 1.0f / (2.0f * sigmaP * sigmaP);
		float sigmaO2i = 1.0f / (2.0f * sigmaO * sigmaO);

		for (int r = 0; r < 180; r++)
		{
			int rr = (f * 180 + r) * filterSize;

			float rad = r * (float)FMath::d2r; 
			float cosphi = cosf(rad);
			float sinphi = sinf(rad);

			float sum = 0.0f;
			for (int y = 0; y < m_blurFiterWidth; y++)
			{
				int yy = rr + y * m_blurFiterWidth;
				for (int x = 0; x < m_blurFiterWidth; x++)
				{
					float px = x - offset;
					float py = y - offset;
					float rx = cosphi * px - sinphi * py;
					float ry = sinphi * px + cosphi * py;
					float c = expf(- rx * rx * sigmaP2i - ry * ry * sigmaO2i);
					m_pBlurFilter[yy + x] = c;
					sum += c;
				}
			}

			float damp = 1.0f / sum;
			for (int i = 0; i < filterSize; i++)
				m_pBlurFilter[rr + i] *= damp;


			if (r == 0 || r == 45 || r == 90)
			{
				qDebug() << endl << "Filter coefficients for angle " << r << endl;
				for (int y = 0; y < m_blurFiterWidth; y++)
				{
					int yy = rr + y * m_blurFiterWidth;
					QString text = QString("L%1: ").arg(y);
					for (int x = 0; x < m_blurFiterWidth; x++)
						text += QString("%1 ").arg(m_pBlurFilter[yy + x], 6, 'f', 4);
					qDebug() << text;
				}
			}
		} // end for (r)
	} // end for (f)
}

// ----------------------------------------------------------------------------------------------------