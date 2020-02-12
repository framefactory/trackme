// ----------------------------------------------------------------------------------------------------
//  Title			FFernTracker.cpp
//  Description		Implementation of class FFernTracker
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 6 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FFernTracker.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FFernTracker
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FFernTracker::FFernTracker()
{
}

FFernTracker::~FFernTracker()
{
}

// Public commands ------------------------------------------------------------------------------------

void FFernTracker::track(const FGLTextureRect& sourceFrame)
{
	m_sourceFrame = sourceFrame;
	_renderFilter();
}

void FFernTracker::reset(const QSize& frameSize)
{
	m_frameSize = frameSize;
	_initGL();
}

// Public queries -------------------------------------------------------------------------------------

const FGLTextureRect& FFernTracker::harris() const
{
	return m_frameBuffer2.texture();
}

// Overrides ------------------------------------------------------------------------------------------

// Internal functions ---------------------------------------------------------------------------------

void FFernTracker::_initGL()
{
	m_overlayRect.setTexCoords(FRect2f(0.0f, 0.0f, m_frameSize.width(), m_frameSize.height()));
	m_overlayRect.create();

	m_frameBuffer1.create(FGLPixelFormat::R32G32B32A32_Float, m_frameSize);
	m_frameBuffer2.create(FGLPixelFormat::R32G32B32A32_Float, m_frameSize);

	FGLShader shOverlay("Shader/overlay.vert");
	FGLShader shGaussian3x3("Shader/gaussian3x3.frag");
	FGLShader shGaussianHorz("Shader/gaussianHorz.frag");
	FGLShader shGaussianVert("Shader/gaussianVert.frag");
	FGLShader shHarrisDerivatives("Shader/harrisDerivatives.frag");
	FGLShader shHarrisCorners("Shader/harrisCornerDetect.frag");
	FGLShader shHarrisSuppression("Shader/harrisSuppression.frag");

	m_prgGaussian3x3.createLinkProgram(shOverlay, shGaussian3x3);
	m_prgGaussianHorz.createLinkProgram(shOverlay, shGaussianHorz);
	m_prgGaussianVert.createLinkProgram(shOverlay, shGaussianVert);
	m_prgHarrisDerivatives.createLinkProgram(shOverlay, shHarrisDerivatives);
	m_prgHarrisCorners.createLinkProgram(shOverlay, shHarrisCorners);
	m_prgHarrisSuppression.createLinkProgram(shOverlay, shHarrisSuppression);
}

void FFernTracker::_renderFilter()
{
	m_frameBuffer1.bindAsTarget();
	m_prgGaussian3x3.bind();
	m_sourceFrame.bind();
	m_overlayRect.draw();
	
	m_frameBuffer2.bindAsTarget();
	m_prgHarrisDerivatives.bind();
	m_frameBuffer1.bindAsSource();
	m_overlayRect.draw();
	
	m_frameBuffer1.bindAsTarget();
	m_prgGaussianHorz.bind();
	m_frameBuffer2.bindAsSource();
	m_overlayRect.draw();

	m_frameBuffer2.bindAsTarget();
	m_prgGaussianVert.bind();
	m_frameBuffer1.bindAsSource();
	m_overlayRect.draw();
	
	m_frameBuffer1.bindAsTarget();
	m_prgHarrisCorners.bind();
	m_frameBuffer2.bindAsSource();
	m_overlayRect.draw();

	m_frameBuffer2.bindAsTarget();
	m_prgHarrisSuppression.bind();
	m_frameBuffer1.bindAsSource();
	m_overlayRect.draw();

	FGLFramebuffer::bindDefault(); // bind default target
	F_GLERROR_ASSERT;
}

// ----------------------------------------------------------------------------------------------------