// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerBase.cpp
//  Description		Implementation of class FLineTrackerBase
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 9 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "levmar/levmar.h"

#include "FLineTrackerBase.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerBase
// ----------------------------------------------------------------------------------------------------

FLineTrackerBase* FLineTrackerBase::s_pActiveTracker = NULL;

// Constructors and destructor ------------------------------------------------------------------------

FLineTrackerBase::FLineTrackerBase()
: m_pCamera(NULL),
  m_pModel(NULL),
  m_blurFilterSteps(360),
  m_blurFiterWidth(9),
  m_sigmaParallel(/* 1.2f */ 3.5f),
  m_sigmaOrthogonal(/*0.3f */ 0.5f),
  m_pBlurFilter(NULL)
{
	_initParameters();
	_calculateBlurFilter(m_sigmaParallel, m_sigmaOrthogonal);

	s_pActiveTracker = this;
}

FLineTrackerBase::~FLineTrackerBase()
{
	F_SAFE_DELETE_ARRAY(m_pBlurFilter);
}

// Public commands ------------------------------------------------------------------------------------

void FLineTrackerBase::track(const FGLTextureRect& sourceFrame)
{
	m_sourceFrame = sourceFrame;
	F_ASSERT(m_sourceFrame.size() == m_frameSize);
	F_ASSERT(m_pCamera);
	F_ASSERT(m_pModel && m_pModel->isValid());

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_pCamera->advanceFrame(0.0f);
	//m_pCamera->advanceFrame(0.65f);
	
	if (!m_trackingEnabled)
		m_pCamera->resetPose();

	_drawDepthModel();
	onProjectModel();

	if (m_trackingEnabled)
		_optimizePose();
}

void FLineTrackerBase::reset(const QSize& frameSize)
{
	m_frameSize = frameSize;
	_resetGL();
	onReset();
}

void FLineTrackerBase::setCamera(FCamera* pCamera)
{
	m_pCamera = pCamera;
}

void FLineTrackerBase::setModel(FLineModel* pModel)
{
	m_pModel = pModel;
}

void FLineTrackerBase::drawSolidModel()
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);

	// Set up model-view-projection transformation for edge model
	FMatrix4f matMV_Extra, matPGL_Extra, matMVPGL_Extra;
	m_pCamera->getModelViewExtra(matMV_Extra);
	m_pCamera->getProjectionGLExtra(matPGL_Extra);
	matMVPGL_Extra = matPGL_Extra * matMV_Extra;

	matMVPGL_Extra.transpose();
	matMV_Extra.transpose();

	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform2.write(matMVPGL_Extra.ptr(), matSize);
	m_bufModelTransform2.write(matMV_Extra.ptr(), matSize, matSize);

	// Draw solid object
	m_prgModelSolid.bind();
	m_prgModelSolid.bindUniformBlock("Transform", 0);
	m_bufModelTransform2.bindUniform(0);
	m_pModel->drawSolidGL();

	glDisable(GL_DEPTH_TEST);
	glDisable(GL_CULL_FACE);
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FLineTrackerBase::initialModel()
{
	_drawInitialPose();
	return m_texOverlayInitialPose;
}

const FGLTextureRect& FLineTrackerBase::fittedModel()
{
	_drawFittedPose();
	return m_texOverlayFittedPose;
}

// Internal functions ---------------------------------------------------------------------------------

void FLineTrackerBase::_resetGL()
{
	m_overlayRect.setTexCoords(m_frameSize);
	m_overlayRect.create();

	m_prgModelSolid.createLinkProgram("Shader/solidModel.vert", "Shader/phong2LightsFixed.frag");
	m_prgModelDepth.createLinkProgram("Shader/meshModel.vert", "Shader/depthPass.frag");
	m_prgModelLine.createLinkProgram("Shader/edgeModel.vert", "Shader/edgePass.frag");

	m_bufModelTransform.createAllocate(16 * sizeof(float), FGLUsage::DynamicDraw);
	m_bufModelTransform2.createAllocate(2 * 16 * sizeof(float), FGLUsage::DynamicDraw);

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
}

void FLineTrackerBase::_initParameters()
{
	m_searchRange = 15.0f;
	m_samplingDistance = 0.1f;
	m_edgeLumaWeight = 1.4f;
	m_edgeChromaWeight = 1.0f;
	m_edgeThreshold = 0.4f;

	m_colorToleranceEnabled = true;
	m_colorPeekDistance = 2.0f;
	m_colorFullEdgeThreshold = 0.2f;
	m_colorHalfEdgeThreshold = 0.6f;

	m_trackingEnabled = true;
}

void FLineTrackerBase::_drawDepthModel()
{
	glEnable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);

	// Set up model-view-projection transformation for edge model
	FMatrix4f matMV_Current, matPGL_Current, matMVPGL_Current;
	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionGLCurrent(matPGL_Current);
	matMVPGL_Current = matPGL_Current * matMV_Current;
	matMVPGL_Current.transpose();
	
	size_t matSize = 16 * sizeof(float);
	m_bufModelTransform.write(matMVPGL_Current.ptr(), matSize);

	// Use color-depth framebuffer to render hidden lines of model
	m_fbModelDepthColor.bind(1);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	// Draw solid object to depth buffer
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
}

void FLineTrackerBase::_optimizePose()
{
	FMatrix4f matMV_Start, matP_Start, matMVP_Start;
	m_pCamera->getModelViewStart(matMV_Start);
	m_pCamera->getProjectionStart(matP_Start);
	matMVP_Start = matP_Start * matMV_Start;
	m_pModel->transform(matMVP_Start);
	m_pModel->calculateBestHypothesis();
	double startCost = m_pModel->averageCost();

	FMatrix4f matMV_Extra, matP_Extra, matMVP_Extra;
	m_pCamera->getModelViewExtra(matMV_Extra);
	m_pCamera->getProjectionExtra(matP_Extra);
	matMVP_Extra = matP_Extra * matMV_Extra;
	m_pModel->transform(matMVP_Extra);
	m_pModel->calculateBestHypothesis();
	double extraCost = m_pModel->averageCost();

	F_TRACE(QString("INIT, start cost: %1, extrapolated cost: %2").arg(startCost).arg(extraCost));

	size_t dataCount = m_pModel->costVectorSize();

	double poseParams[] = { 0.0, 0.0, 0.0, 0.0, 0.0, 0.0 };
	double opts[] = { 1e-3, 1e-17, 1e-17, 1e-17, 1e-3 }; // tau, eps1, eps2, eps3, delta
	double lmInfo[LM_INFO_SZ];

	int iter = dlevmar_dif(FLineTrackerBase::sLevmarUpdate,
		poseParams, NULL, 6, dataCount, 50, opts, lmInfo, NULL, NULL, 0, (void*)this);

	//m_pCamera->updatePose(poseParams);
	//m_pModel->transform(m_pCamera->modelViewProjectionResultMatrix());
	//m_pModel->calculateBestHypothesis();
	double optCost = m_pModel->averageCost();

	F_TRACE(QString("OPTIMIZED, #iter: %1, cost: %2, #data: %3, term reason: %4")
		.arg(iter).arg(optCost).arg(dataCount).arg(lmInfo[6]));

	/*
	F_TRACE(QString("Final Pose: Tr(%1, %2, %3), Rot(%4, %5, %6)")
		.arg(poseParams[0], 0, 'g', 3).arg(poseParams[1], 0, 'g', 3).arg(poseParams[2], 0, 'g', 3)
		.arg(poseParams[3], 0, 'g', 3).arg(poseParams[4], 0, 'g', 3).arg(poseParams[5], 0, 'g', 3));
	*/
	/*
	double smoothFactor;
	if (optCost > 2.0)
		smoothFactor = FMath::limitedLerp(optCost, 2.0, 4.0, 0.9, 0.0);
	else
		smoothFactor = FMath::limitedLerp(optCost, 0.0, 1.5, 0.3, 0.9);
	
	m_pCamera->smoothResult(smoothFactor);

	m_pModel->transform(m_pCamera->modelViewProjectionResultMatrix());
	m_pModel->calculateBestHypothesis();
	double smoothedCost = m_pModel->averageCost();
	F_TRACE(QString("SMOOTHED, cost: %1, factor: %2").arg(smoothedCost).arg(smoothFactor));
	*/
}

void FLineTrackerBase::_levmarUpdate(double* pPoseParams, double* hx, int m, int n)
{
	F_ASSERT(n == m_pModel->costVectorSize());

	FMatrix4f matMV_Current, matP_Current, matMVP_Current;

	m_pCamera->updatePose(pPoseParams, false);

	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionCurrent(matP_Current);
	matMVP_Current = matP_Current * matMV_Current;

	m_pModel->transform(matMVP_Current);
	m_pModel->calculateBestHypothesis();
	m_pModel->getCostVector(hx);
}

void FLineTrackerBase::_levmarJacobian(double* pParams, double* jac, int m, int n)
{
	F_ASSERT(!"Jacobian not implemented correctly yet");
}

void FLineTrackerBase::_drawInitialPose()
{
	m_fbOverlayInitialPose.bind();
	
	glClearColor(1.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_prgModelLine.bind();
	m_prgModelLine.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);
	size_t matSize = 16 * sizeof(float);

	// Draw line model with base pose from previous frame
	FMatrix4f matMV_Start, matPGL_Start, matMVPGL_Start;
	m_pCamera->getModelViewStart(matMV_Start);
	m_pCamera->getProjectionGLStart(matPGL_Start);
	matMVPGL_Start = matPGL_Start * matMV_Start;
	matMVPGL_Start.transpose();
	m_bufModelTransform.write(matMVPGL_Start.ptr(), matSize);
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

void FLineTrackerBase::_drawFittedPose()
{
	FMatrix4f matMV_Current, matP_Current, matMVP_Current;
	m_pCamera->getModelViewCurrent(matMV_Current);
	m_pCamera->getProjectionCurrent(matP_Current);
	matMVP_Current = matP_Current * matMV_Current;
	m_pModel->transform(matMVP_Current);

	m_fbOverlayFittedPose.bind();
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);
	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());

	m_canvasFittedPose.clear();
	m_pModel->drawCandidates(m_canvasFittedPose);
	m_canvasFittedPose.draw();

	FGLFramebuffer::bindDefault();
}

void FLineTrackerBase::_reprojectModel()
{
	F_ASSERT(false);
}

void FLineTrackerBase::_calculateBlurFilter(float sigmaP, float sigmaO)
{
	size_t filterSize = m_blurFiterWidth * m_blurFiterWidth;
	size_t numElements = m_blurFilterSteps * filterSize;

	F_SAFE_DELETE_ARRAY(m_pBlurFilter);
	m_pBlurFilter = new float[numElements];

	float sigmaP2i = 1.0f / (2.0f * sigmaP * sigmaP);
	float sigmaO2i = 1.0f / (2.0f * sigmaO * sigmaO);
	
	float offset = (float)(m_blurFiterWidth / 2);

	for (int r = 0; r < 360; r++)
	{
		int rr = r * filterSize;

		float rad = r * float(F_PI) / 180.0f; 
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
	}
}

// ----------------------------------------------------------------------------------------------------