// ----------------------------------------------------------------------------------------------------
//  Title			FStreamViewer.cpp
//  Description		Implementation of class FStreamViewer
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 13 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FlowGL.h"

#include "FStreamEngine.h"

#include "FStreamViewer.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamViewer
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamViewer::FStreamViewer(const FGLContext& context, FStreamEngine* pEngine, QObject* pParent /* = NULL */)
: QObject(pParent),
  m_context(context),
  m_pEngine(pEngine),
  m_viewportRect(0, 0, 768, 576),
  m_frameSize(768, 576)
{
	F_ASSERT(context.isValid());
	F_ASSERT(m_pEngine);

	F_VERIFY(_initGL());
	F_VERIFY(_resetGL());
}

FStreamViewer::~FStreamViewer()
{
}

// Public commands ------------------------------------------------------------------------------------

void FStreamViewer::reset(const QSize& frameSize)
{
	F_DEBUG("FStreamViewer", QString("Reset - New frame size: %1 x %2")
		.arg(frameSize.width()).arg(frameSize.height()));

	m_frameSize = frameSize;
	F_VERIFY(_resetGL());
}

void FStreamViewer::redraw()
{
	FGLTextureRect texTrackerInitialModel;
	FGLTextureRect texTrackerFittedModel;
	FGLTextureRect texTrackerReferenceColors;
	FGLTextureRect texAugmentedImage;
	FGLTextureRect texDetection;

	switch(m_viewMode)
	{
	case FStreamViewMode::InitialModel:
		texTrackerInitialModel = m_pEngine->trackerInitialModel();
		break;
	case FStreamViewMode::FittedModel:
		texTrackerFittedModel = m_pEngine->trackerFittedModel();
		break;
	case FStreamViewMode::ReferenceColors:
		texTrackerReferenceColors = m_pEngine->trackerReferenceColors();
		break;
	case FStreamViewMode::AugmentedImage:
		//texAugmentedImage = m_pEngine->drawAugmentedImage();
		break;
	}

	FGLFramebuffer::bindDefault(); // render to the screen

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	glClearColor(0.1f, 0.1f, 0.1f, 1.0f);

	glDisable(GL_CULL_FACE);
	glDisable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);


	switch(m_viewMode)
	{
	case FStreamViewMode::Input:
		m_prgPlainImage.bind();
		m_pEngine->currentFrame().bind(0);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::SolidModel:
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_pEngine->drawSolidModel();
		break;

	case FStreamViewMode::DepthPass:
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		m_prgDepthPass.bind();
		glUniform1i(m_suDepthImage, 0);
		m_pEngine->trackerDepthPass().bind(0);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::InitialModel:
		m_prgCanvasOverlay.bind();
		glUniform1i(m_suSourceImage, 0);
		glUniform1i(m_suOverlayImage, 1);
		glUniform1f(m_uImageBlendFactor, 0.6f);
		m_pEngine->currentFrame().bind(0);
		texTrackerInitialModel.bind(1);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::FittedModel:
		m_prgCanvasOverlay.bind();
		glUniform1i(m_suSourceImage, 0);
		glUniform1i(m_suOverlayImage, 1);
		glUniform1f(m_uImageBlendFactor, 0.75f);
		m_pEngine->currentFrame().bind(0);
		texTrackerFittedModel.bind(1);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::ReferenceColors:
		m_prgCanvasOverlay.bind();
		glUniform1i(m_suSourceImage, 0);
		glUniform1i(m_suOverlayImage, 1);
		glUniform1f(m_uImageBlendFactor, 0.6f);
		m_pEngine->currentFrame().bind(0);
		texTrackerReferenceColors.bind(1);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::AugmentedImage:
		m_prgPlainImage.bind();
		//m_prgCanvasOverlay.bind();
		//glUniform1i(m_suSourceImage, 0);
		//glUniform1i(m_suOverlayImage, 1);
		//glUniform1f(m_uImageBlendFactor, 1.0f);
		m_pEngine->frameAt(2).bind(0);
		//texAugmentedImage.bind(1);
		m_overlayRect.draw();
		m_pEngine->drawAugmentedImage();
		break;

	case FStreamViewMode::CannyEdges:
		m_prgPlainImage.bind();
		m_pEngine->cannyEdges().bind(0);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::DistanceTransform:
		m_prgViewDistanceTransform.bind();
		glUniform1i(m_suDTImage, 0);
		m_pEngine->distanceTransform().bind(0);
		m_overlayRect.draw();
		break;

	case FStreamViewMode::Contours:
		if (m_pEngine->isPoseDetectorIdle())
		{
			m_prgViewContours.bind();
			glUniform1i(m_suContourData, 0);
			m_pEngine->contourView().bind(0);
			m_overlayRect.draw();
			m_canvas.clear();
			m_pEngine->drawContourStatistics(m_canvas);
			m_canvas.draw();
			m_pEngine->drawDetectionMap();
		}
		break;

	case FStreamViewMode::DetectedPose:
		if (m_pEngine->isPoseDetectorIdle())
		{
			m_prgPlainImage.bind();
			m_pEngine->currentFrame().bind(0);
			m_overlayRect.draw();
			m_pEngine->drawDetectionMap();
			m_pEngine->drawDetectedPose();
		}
		break;

	case FStreamViewMode::HarrisCorners:
		/*
		m_prgViewHarris.bind();
		glUniform1i(m_suHarrisSource, 0);
		glUniform1i(m_suHarrisCorners, 1);
		m_pEngine->currentFrame().bind(0);
		//m_pEngine->harrisCorners().bind(1);
		m_overlayRect.draw();
		*/
		break;

	default:
		F_ASSERT(false);
		break;
	}

	m_context.swapBuffers();
}

void FStreamViewer::clear()
{
	glViewport(m_viewportRect.left(), m_viewportRect.top(),
		m_viewportRect.width(), m_viewportRect.height());
	glClearColor(0.0f, 0.0f, 0.0f, 1.0f);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	m_context.swapBuffers();
}

void FStreamViewer::setMode(quint32 viewMode)
{
	m_viewMode = (FStreamViewMode::state_t)viewMode;
}

// Public slots ---------------------------------------------------------------------------------------

void FStreamViewer::setWindowSize(const QSize& newSize)
{
	F_DEBUG("FStreamViewer", QString("Render widget size changed: %1 x %2")
		.arg(newSize.width()).arg(newSize.height()));
	m_canvasSize = newSize;
	m_viewportRect = QRect(QPoint(0, 0), m_canvasSize);
}

// Public queries -------------------------------------------------------------------------------------

void FStreamViewer::readPixels(void* pData)
{
	glReadPixels(0, 0, m_frameSize.width(), m_frameSize.height(), FGLDataFormat::BGRA,
		FGLDataType::UnsignedByte, pData);
}

// Internal functions ---------------------------------------------------------------------------------

bool FStreamViewer::_initGL()
{
	FGLShader shadViewOverlay("Shader/overlay.vert");

	m_prgPlainImage.create();
	m_prgPlainImage.attachShader(shadViewOverlay);
	m_prgPlainImage.addShaderFromFile("Shader/viewPlainImage.frag", FGLShaderType::FragmentShader);
	m_prgPlainImage.bindAttribLocation(0, "vVertexPosition");
	m_prgPlainImage.bindAttribLocation(1, "vVertexTexCoord");
	m_prgPlainImage.bindFragDataLocation(0, "vOutColor");
	m_prgPlainImage.link();

	m_prgCanvasOverlay.create();
	m_prgCanvasOverlay.attachShader(shadViewOverlay);
	m_prgCanvasOverlay.addShaderFromFile("Shader/viewCanvasOverlay.frag", FGLShaderType::FragmentShader);
	m_prgCanvasOverlay.bindAttribLocation(0, "vVertexPosition");
	m_prgCanvasOverlay.bindAttribLocation(1, "vVertexTexCoord");
	m_prgCanvasOverlay.bindFragDataLocation(0, "vOutColor");
	m_prgCanvasOverlay.link();
	m_suSourceImage = m_prgCanvasOverlay.getUniformLocation("sSourceImage");
	m_suOverlayImage = m_prgCanvasOverlay.getUniformLocation("sOverlayImage");
	m_uImageBlendFactor = m_prgCanvasOverlay.getUniformLocation("imageBlendFactor");

	m_prgEdgeModel.create();
	m_prgEdgeModel.attachShader(shadViewOverlay);
	m_prgEdgeModel.addShaderFromFile("Shader/viewEdgeModel.frag", FGLShaderType::FragmentShader);
	m_prgEdgeModel.bindAttribLocation(0, "vVertexPosition");
	m_prgEdgeModel.bindAttribLocation(1, "vVertexTexCoord");
	m_prgEdgeModel.bindFragDataLocation(0, "vOutColor");
	m_prgEdgeModel.link();

	m_prgViewHarris.create();
	m_prgViewHarris.attachShader(shadViewOverlay);
	m_prgViewHarris.addShaderFromFile("Shader/viewHarrisCorners.frag", FGLShaderType::FragmentShader);
	m_prgViewHarris.bindAttribLocation(0, "vVertexPosition");
	m_prgViewHarris.bindAttribLocation(1, "vVertexTexCoord");
	m_prgViewHarris.bindFragDataLocation(0, "vOutColor");
	m_prgViewHarris.link();
	m_suHarrisSource = m_prgViewHarris.getUniformLocation("sSourceFrame");
	m_suHarrisCorners = m_prgViewHarris.getUniformLocation("sHarrisCorners");

	m_prgDepthPass.create();
	m_prgDepthPass.attachShader(shadViewOverlay);
	m_prgDepthPass.addShaderFromFile("Shader/viewDepthPass.frag", FGLShaderType::FragmentShader);
	m_prgDepthPass.bindAttribLocation(0, "vVertexPosition");
	m_prgDepthPass.bindAttribLocation(1, "vVertexTexCoord");
	m_prgDepthPass.bindFragDataLocation(0, "vOutColor");
	m_prgDepthPass.link();
	m_suDepthImage = m_prgDepthPass.getUniformLocation("sDepthImage");

	m_prgViewDistanceTransform.create();
	m_prgViewDistanceTransform.attachShader(shadViewOverlay);
	m_prgViewDistanceTransform.addShaderFromFile("Shader/viewDistanceTransform.frag", FGLShaderType::FragmentShader);
	m_prgViewDistanceTransform.bindAttribLocation(0, "vVertexPosition");
	m_prgViewDistanceTransform.bindAttribLocation(1, "vVertexTexCoord");
	m_prgViewDistanceTransform.bindFragDataLocation(0, "vOutColor");
	m_prgViewDistanceTransform.link();
	m_suDTImage = m_prgViewDistanceTransform.getUniformLocation("sDistanceTransform");

	m_prgViewContours.create();
	m_prgViewContours.attachShader(shadViewOverlay);
	m_prgViewContours.addShaderFromFile("Shader/viewContours.frag", FGLShaderType::FragmentShader);
	m_prgViewContours.bindAttribLocation(0, "vVertexPosition");
	m_prgViewContours.bindAttribLocation(1, "vVertexTexCoord");
	m_prgViewContours.bindFragDataLocation(0, "vOutColor");
	m_prgViewContours.link();
	m_suContourData = m_prgViewContours.getUniformLocation("sContourData");

	return F_GLNOERROR;
}

bool FStreamViewer::_resetGL()
{
	m_overlayRect.setTexCoords(m_frameSize);
	m_overlayRect.create();

	return F_GLNOERROR;
}

// ----------------------------------------------------------------------------------------------------