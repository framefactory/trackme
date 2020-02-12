// ----------------------------------------------------------------------------------------------------
//  Title			FDistanceTransform.cpp
//  Description		Implementation of class FDistanceTransform
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-14 22:42:33 +0200 (So, 14 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"
#include "FBit.h"

#include "FDistanceTransform.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FDistanceTransform
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FDistanceTransform::FDistanceTransform()
: m_isValid(false),
  m_imageSize(0, 0),
  m_maxStepSize(0),
  m_stepCount(0),
  m_edgeThresholdLow(0.02f),
  m_edgeThresholdHigh(0.07f)
{
	F_VERIFY(_initGL());
}

FDistanceTransform::~FDistanceTransform()
{
}

// Public commands ------------------------------------------------------------------------------------

void FDistanceTransform::calculateDt(const FGLTextureRect& inputImage)
{
	_runDistanceTransform(inputImage);
}

void FDistanceTransform::calculateCannyDt(const FGLTextureRect& inputImage)
{
	_runCanny(inputImage);
	_runDistanceTransform(m_texBuffer[1]);
}

bool FDistanceTransform::reset(const QSize& imageSize)
{
	F_ASSERT(!imageSize.isEmpty());
	if (imageSize.isEmpty())
		return false;

	m_imageSize = imageSize;
	int maxSize = fMax(imageSize.width(), imageSize.height()) >> 1;
	for(m_maxStepSize = 1; m_maxStepSize < maxSize; m_maxStepSize <<= 1)
		;
	for(m_stepCount = 1; maxSize > 0; maxSize >>= 1)
		m_stepCount++;

	m_isValid = _resetGL();
	return m_isValid;
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FDistanceTransform::result() const
{
	F_ASSERT(m_texBuffer[0].isValid());
	return m_texBuffer[(m_stepCount + 1) % 2];
}

void FDistanceTransform::getResult(FDTPixel* pDTImage)
{
	F_ASSERT(m_texBuffer[0].isValid());
	m_texBuffer[(m_stepCount + 1) % 2].read(FGLDataFormat::RGBA, FGLDataType::Float, pDTImage);
}

// Internal functions ---------------------------------------------------------------------------------

void FDistanceTransform::_runCanny(const FGLTextureRect& inputImage)
{
	// run 1d horizontal gaussian kernel (result in buffer 1)
	m_prgGaussianHorz.bind();
	m_fbBuffer[1].bind(1);
	inputImage.bind();
	m_overlayRect.draw();

	// run 1d vertical gaussian kernel (result in buffer 0)
	m_prgGaussianVert.bind();
	m_fbBuffer[0].bind(1);
	m_texBuffer[1].bind();
	m_overlayRect.draw();

	// run canny edge detection step (result in buffer 1)
	m_prgCannyDetect.bind();
	m_fbBuffer[1].bind(1);
	m_texBuffer[0].bind();
	m_overlayRect.draw();

	// run canny edge suppression step (result in buffer 0)
	m_prgCannySuppress.bind();
	m_fbBuffer[0].bind(1);
	m_texBuffer[1].bind();
	m_overlayRect.draw();

	// run canny edge threshold step (result in buffer 1)
	m_prgCannyThreshold.bind();
	glUniform1f(m_uEdgeThresholdLow, m_edgeThresholdLow);
	glUniform1f(m_uEdgeThresholdHigh, m_edgeThresholdHigh);
	m_fbBuffer[1].bind(1);
	m_prgCannyThreshold.setSamplerUniform(0, "sEdges");
	m_texBuffer[0].bind(0);
	m_overlayRect.draw();

	// run edge thin pass 1 (result in buffer 0)
	m_prgCannyReduce1.bind();
	m_fbBuffer[0].bind(1);
	m_texBuffer[1].bind();
	m_overlayRect.draw();

	// run edge thin pass 2 (result in buffer 1)
	m_prgCannyReduce2.bind();
	m_fbBuffer[1].bind(1);
	m_texBuffer[0].bind();
	m_overlayRect.draw();
}

void FDistanceTransform::_runDistanceTransform(const FGLTextureRect& inputImage)
{
	F_ASSERT(isValid());
	if (!isValid())
		return;

	glViewport(0, 0, m_imageSize.width(), m_imageSize.height());
	m_samplerBorder.bind(0);

	// set step size.
	m_prgJumpFloodInit.bind();
	glUniform1i(m_uImage, 0);
	glUniform1f(m_uStepSize, (float)m_maxStepSize);

	// execute first step of jump flood algorithm, render to buffer 0
	//F_TRACE(QString("JumpFlood - Render to #%1, step size %2").arg(0).arg(m_maxStepSize));
	m_fbBuffer[0].bind(1);
	inputImage.bind(0);
	m_overlayRect.draw();

	int stepSize = m_maxStepSize >> 1;
	m_prgJumpFloodStep.bind();
	glUniform1i(m_uImage, 0);

	// execute remaining steps, render to alternating buffers 1/0
	for (int i = 1; i < m_stepCount; i++)
	{
		int stepSource = (i + 1) % 2;
		int stepTarget = i % 2;

		//F_TRACE(QString("JumpFlood - Render to #%1, step size %2").arg(stepTarget).arg(stepSize));
		glUniform1f(m_uStepSize, (float)stepSize);
		m_fbBuffer[stepTarget].bind(1);
		m_texBuffer[stepSource].bind(0);
		m_overlayRect.draw();
		stepSize = stepSize >> 1;
	}

	m_samplerBorder.unbind(0);
}

bool FDistanceTransform::_initGL()
{
	for (int i = 0; i < 2; i++)
	{
		m_texBuffer[i].create();
		m_fbBuffer[i].create();
	}

	FGLShader shOverlay("Shader/overlay.vert");

	// canny

	FGLShader shGaussianHorz("Shader/gaussianHorz.frag");
	FGLShader shGaussianVert("Shader/gaussianVert.frag");
	FGLShader shCannyEdgeDetect("Shader/cannyEdgeDetect.frag");
	FGLShader shCannyEdgeSuppress("Shader/cannyEdgeSuppress.frag");
	FGLShader shCannyEdgeReduce1("Shader/cannyEdgeReduce1.frag");
	FGLShader shCannyEdgeReduce2("Shader/cannyEdgeReduce2.frag");
	FGLShader shCannyEdgeThreshold("Shader/cannyEdgeThreshold.frag");

	m_prgGaussianHorz.createLinkProgram(shOverlay, shGaussianHorz);
	m_prgGaussianVert.createLinkProgram(shOverlay, shGaussianVert);
	m_prgCannyDetect.createLinkProgram(shOverlay, shCannyEdgeDetect);
	m_prgCannySuppress.createLinkProgram(shOverlay, shCannyEdgeSuppress);
	m_prgCannyReduce1.createLinkProgram(shOverlay, shCannyEdgeReduce1);
	m_prgCannyReduce2.createLinkProgram(shOverlay, shCannyEdgeReduce2);
	m_prgCannyThreshold.createLinkProgram(shOverlay, shCannyEdgeThreshold);

	m_uEdgeThresholdLow = m_prgCannyThreshold.getUniformLocation("edgeThresholdLow");
	m_uEdgeThresholdHigh = m_prgCannyThreshold.getUniformLocation("edgeThresholdHigh");

	// distance transform

	m_samplerBorder.create();
	m_samplerBorder.setFilter(FGLFilterType::Nearest, FGLFilterType::Nearest);
	m_samplerBorder.setBorderValues(FVector4f(0.0f, 0.0f, 100000.0f, 0.0f));
	m_samplerBorder.setWrap(FGLWrapMode::Border);

	FGLShader shJumpFloodInit("Shader/jumpFloodInit.frag");
	FGLShader shJumpFloodStep("Shader/jumpFloodStep.frag");

	m_prgJumpFloodInit.createLinkProgram(shOverlay, shJumpFloodInit);
	m_prgJumpFloodStep.createLinkProgram(shOverlay, shJumpFloodStep);

	F_ASSERT(m_prgJumpFloodInit.isLinked());
	F_ASSERT(m_prgJumpFloodStep.isLinked());

	m_uImage = m_prgJumpFloodStep.getUniformLocation("sImage");
	m_uStepSize = m_prgJumpFloodStep.getUniformLocation("stepSize");

	return F_GLNOERROR;
}

bool FDistanceTransform::_resetGL()
{
	F_ASSERT(!m_imageSize.isEmpty());
	if (m_imageSize.isEmpty())
		return false;

	for (int i = 0; i < 2; i++)
	{
		m_texBuffer[i].allocate(FGLPixelFormat::R32G32B32A32_Float, m_imageSize);
		m_fbBuffer[i].attachColorTexture(m_texBuffer[i], 0);
		F_ASSERT(m_fbBuffer[i].checkStatus());
	}

	m_overlayRect.setTexCoords(m_imageSize);
	m_overlayRect.create();

	return F_GLNOERROR;
}

// ----------------------------------------------------------------------------------------------------