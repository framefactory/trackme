// ----------------------------------------------------------------------------------------------------
//  Title			FLineTrackerGPU2.cpp
//  Description		Implementation of class FLineTrackerGPU2
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 9 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FLineTrackerGPU2.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FLineTrackerGPU2
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FLineTrackerGPU2::FLineTrackerGPU2()
: m_pSearchResult(NULL),
  m_transferSize(640, 40)
{
}

FLineTrackerGPU2::~FLineTrackerGPU2()
{
	F_SAFE_DELETE_ARRAY(m_pSearchResult);
}

// Overrides ------------------------------------------------------------------------------------------

void FLineTrackerGPU2::onReset()
{
	_resetGL();
}

void FLineTrackerGPU2::onProjectModel()
{
	// statistics
	m_sampleCount = 0;
	m_candidateCount = 0;
	m_pixelCount = 0;

	_projectModel();
}

const FGLTextureRect& FLineTrackerGPU2::onProcessedFrame(size_t index) const
{
	if (index == 0)
		return m_fbTexModelSearchResult; // hidden line model
	else if (index == 1)
		return m_texOverlayFittedPose; // result of edge search

	return m_sourceFrame;
}

// Internal functions ---------------------------------------------------------------------------------

void FLineTrackerGPU2::_resetGL()
{
	m_transferRect.setTexCoords(m_transferSize);
	m_transferRect.create();

	FGLShader shOverlay("Shader/overlay.vert");

	size_t numBytes = m_blurFiterWidth * m_blurFiterWidth * m_blurFilterSteps * sizeof(float);
	m_bufBlurFilter.createInitialize(m_pBlurFilter, numBytes, FGLUsage::StaticDraw);
	m_texBlurFilter.createAttach(m_bufBlurFilter, FGLPixelFormat::R32_Float);

	m_prgModelDepth.createLinkProgram("Shader/meshModel.vert", "Shader/depthPass.frag");
	m_prgModelSample.createLinkProgram("Shader/sampleModel.vert",
		"Shader/sampleModel_2.geom", "Shader/sampleModel_2.frag");
	m_suHiddenLineModel = m_prgModelSample.getUniformLocation("sDepthModel");
	m_suImage = m_prgModelSample.getUniformLocation("sImage");
	m_suFilterKernel = m_prgModelSample.getUniformLocation("sFilterKernel");
	m_uSearchRange = m_prgModelSample.getUniformLocation("searchDistance");
	m_uSamplingDistance = m_prgModelSample.getUniformLocation("minSampleDistance");
	m_uFilterWidth = m_prgModelSample.getUniformLocation("filterWidth");
	m_uEdgeLumaWeight = m_prgModelSample.getUniformLocation("edgeLumaWeight");
	m_uEdgeChromaWeight = m_prgModelSample.getUniformLocation("edgeChromaWeight");
	m_uEdgeThreshold = m_prgModelSample.getUniformLocation("edgeThreshold");
	m_uColorPeekDistance = m_prgModelSample.getUniformLocation("colorPeekDistance");
	m_uColorFullEdgeThreshold = m_prgModelSample.getUniformLocation("colorFullEdgeThreshold");
	m_uColorHalfEdgeThreshold = m_prgModelSample.getUniformLocation("colorHalfEdgeThreshold");

	FGLShader shSampleEdgeSuppress("Shader/sampleEdgeSuppress_2.frag");
	m_prgModelEdgeSuppress.createLinkProgram(shOverlay, shSampleEdgeSuppress);
	m_uImageSize = m_prgModelSample.getUniformLocation("imageSize");
	m_uTransferSize = m_prgModelSample.getUniformLocation("transferSize");
	m_bufModelTransform.createAllocate(2 * 16 * sizeof(float), FGLUsage::DynamicDraw);

	// 2 color frame buffers for storing search results
	m_fbTexModelSearchIntermediate.createAllocate(FGLPixelFormat::R32G32B32A32_Float, m_transferSize);
	m_fbModelSearchIntermediate.create();
	m_fbModelSearchIntermediate.attachColorTexture(m_fbTexModelSearchIntermediate, 0);
	F_ASSERT(m_fbModelSearchIntermediate.checkStatus());

	m_fbTexModelSearchResult.createAllocate(FGLPixelFormat::R32G32B32A32_Float, m_transferSize);
	m_fbModelSearchResult.create();
	m_fbModelSearchResult.attachColorTexture(m_fbTexModelSearchResult, 0);
	F_ASSERT(m_fbModelSearchResult.checkStatus());

	// Host memory for search results
	F_SAFE_DELETE_ARRAY(m_pSearchResult);
	m_pSearchResult = new FPixelRGBA32f[m_transferSize.width() * m_transferSize.height()];
}

void FLineTrackerGPU2::_projectModel()
{
	// Draw search lines

	// Use search target framebuffer

	m_fbModelSearchIntermediate.bind(1);
	glViewport(0, 0, m_transferSize.width(), m_transferSize.height());
	glClearColor(0.0f, 0.0f, 0.0f, 0.0f);
	glClear(GL_COLOR_BUFFER_BIT);

	m_sourceFrame.setFilter(FGLFilterType::Linear, FGLFilterType::Linear);
	m_prgModelSample.bind();

	// Geometry shader setup
	glUniform2f(m_uImageSize, m_frameSize.width(), m_frameSize.height());
	glUniform2f(m_uTransferSize, m_transferSize.width(), m_transferSize.height());
	glUniform1f(m_uSamplingDistance, m_samplingDistance);
	glUniform1f(m_uSearchRange, m_searchRange);
	m_prgModelSample.bindUniformBlock("Transform", 0);
	m_bufModelTransform.bindUniform(0);

	// use hidden line image to decide if search line should be drawn
	glUniform1i(m_suHiddenLineModel, 0);
	m_fbTexModelDepth.bind(0); 

	// Fragment shader setup
	glUniform1i(m_uFilterWidth, m_blurFiterWidth);
	glUniform1f(m_uEdgeLumaWeight, m_edgeLumaWeight);
	glUniform1f(m_uEdgeChromaWeight, m_edgeChromaWeight);
	glUniform1f(m_uEdgeThreshold, m_edgeThreshold);
	glUniform1f(m_uColorPeekDistance, m_colorPeekDistance);
	glUniform1f(m_uColorFullEdgeThreshold, m_colorFullEdgeThreshold);
	glUniform1f(m_uColorHalfEdgeThreshold, m_colorHalfEdgeThreshold);
	glUniform1i(m_suImage, 1);
	m_sourceFrame.bind(1);
	glUniform1i(m_suFilterKernel, 2);
	m_texBlurFilter.bind(2);

	// Execute
	m_pModel->drawLinesGL();
	m_sourceFrame.setFilter(FGLFilterType::Nearest, FGLFilterType::Nearest);

	// Refine found edges: non-maximum suppression
	m_fbModelSearchResult.bind();
	m_prgModelEdgeSuppress.bind();
	m_fbTexModelSearchIntermediate.bind(0);
	m_transferRect.draw();

	glViewport(0, 0, m_frameSize.width(), m_frameSize.height());
	FGLFramebuffer::bindDefault(); // bind default target
	F_GLERROR_ASSERT;

	// Retrieve edge candidates from rendered image
	F_ASSERT(m_pSearchResult);
	m_fbTexModelSearchResult.read(FGLDataFormat::RGBA, FGLDataType::Float, m_pSearchResult);

	int n = m_transferSize.width() * m_transferSize.height();
	int nx = m_transferSize.width();
	int ny = m_transferSize.height();

	m_pModel->beginAddCandidates();
	bool colTolEnabled = m_colorToleranceEnabled;

	for (int y = 0; y < ny; y++)
	{
		int yy = y * nx;
		for (int x = 0; x < nx; x++)
		{
			FPixelRGBA32f& p = m_pSearchResult[yy + x];
			if (p.r > 0.0f) // edge candidate found
			{
				int line = y / 40;
				int id = line * nx + x;
				int edgeId = id / 24;
				int sampleId = id % 24;

				m_pModel->addCandidate(edgeId, sampleId, FVector2f(p.b, p.a),
					p.r, colTolEnabled ? p.g : 1.0f);

				//m_candidateCount++; // statistics
				//m_pixelCount++; // statistics
			}
			else if (p.a > 0.0f) // not an edge candidate but a sample pixel
			{
				//m_pixelCount++; // statistics
			}
		}
	}
	m_pModel->endAddCandidates();

	//m_sampleCount = m_pModel->countSamples(); // statistics

	//m_pModel->setBaseTransform(m_pCamera->modelViewProjectionMatrix());
	//m_pModel->updateTransform();
}

// ----------------------------------------------------------------------------------------------------