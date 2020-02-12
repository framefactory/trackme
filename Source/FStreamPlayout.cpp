// ----------------------------------------------------------------------------------------------------
//  Title			FStreamPlayout.cpp
//  Description		Implementation of class FStreamPlayout
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/31 $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FlowGLDefs.h"
#include "FGLFramebuffer.h"

#include "FStreamPlayout.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamPlayout
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamPlayout::FStreamPlayout(QObject* pParent /* = NULL */)
: QThread(pParent),
  m_wantExit(false),
  m_readIndex(0),
  m_writeIndex(0),
  m_playoutFrameIndex(0),
  m_jpegQuality(90)
{
	m_frameQueue.resize(32);
}

FStreamPlayout::~FStreamPlayout()
{
}

// Public commands ------------------------------------------------------------------------------------

void FStreamPlayout::start(const QString& filePath, const QSize& frameSize)
{
	F_ASSERT(!isRunning());
	if (isRunning())
		return;

	F_ASSERT(!frameSize.isEmpty());
	F_ASSERT(!filePath.isEmpty());

	for (size_t i = 0; i < m_frameQueue.size(); ++i)
		m_frameQueue[i] = QImage(frameSize.width(), frameSize.height(),	QImage::Format_ARGB32);
	m_frameBuffer = QImage(frameSize.width(), frameSize.height(),	QImage::Format_ARGB32);

	m_readIndex = m_writeIndex = 0;
	m_playoutFrameIndex = 0;

	m_frameSize = frameSize;
	m_baseFilePath = filePath;

	m_wantExit = false;
	QThread::start(QThread::LowPriority);
}

void FStreamPlayout::stop()
{
	if (isRunning())
	{
		m_wantExit = true;
		m_waitForFrame.wakeAll();
		wait(2000);
		terminate();
		wait();

		for (size_t i = 0; i < m_frameQueue.size(); ++i)
			m_frameQueue[i] = QImage();
	}
}

void FStreamPlayout::writeFrame(const FGLTextureRect& frame)
{
	m_objectLock.lock();
	
	size_t writeIndex = (m_writeIndex + 1) % m_frameQueue.size();
	if (writeIndex == m_readIndex)
	{
		m_objectLock.unlock();
		return;
	}

	m_writeIndex = writeIndex;
	frame.read(FGLDataFormat::BGRA, FGLDataType::Float, m_frameQueue[m_writeIndex].bits());

	m_objectLock.unlock();
	m_waitForFrame.wakeAll();
}

void FStreamPlayout::writeBackbuffer()
{
	m_objectLock.lock();

	size_t writeIndex = (m_writeIndex + 1) % m_frameQueue.size();
	if (writeIndex == m_readIndex)
	{
		m_objectLock.unlock();
		return;
	}

	m_objectLock.unlock();

	glFinish();
	FGLFramebuffer::bindDefault();
	glBindBuffer(GL_PIXEL_PACK_BUFFER, 0);
	glReadPixels(0, 0, m_frameSize.width(), m_frameSize.height(), FGLDataFormat::BGRA,
		FGLDataType::UnsignedByte, m_frameQueue[writeIndex].bits());
	glFinish();
	F_GLERROR_ASSERT;

	m_objectLock.lock();
	m_writeIndex = writeIndex;
	m_objectLock.unlock();

	m_waitForFrame.wakeAll();
}

// Overrides ------------------------------------------------------------------------------------------

void FStreamPlayout::run()
{
	m_objectLock.lock();

	while(true)
	{
		if (m_wantExit)
		{
			m_objectLock.unlock();
			return;
		}

		m_waitForFrame.wait(&m_objectLock);

		while(m_readIndex != m_writeIndex)
		{
			m_readIndex = (m_readIndex + 1) % m_frameQueue.size();
			m_objectLock.unlock();

			size_t stride = m_frameQueue[m_readIndex].bytesPerLine();
			size_t lines = m_frameQueue[m_readIndex].height();
			quint8* pSrc = m_frameQueue[m_readIndex].bits();
			quint8* pDst = m_frameBuffer.bits();

			for (size_t y0 = 0, y1 = lines - 1; y0 < lines; ++y0, --y1)
			{
				size_t yy0 = y0 * stride;
				size_t yy1 = y1 * stride;
				for (size_t x = 0; x < stride; ++x)
					pDst[yy1 + x] = pSrc[yy0 + x];
			}

			QString filePath = QString("%1_%2.jpg")
				.arg(m_baseFilePath).arg((int)m_playoutFrameIndex, 4, 10, QLatin1Char('0'));
			m_frameBuffer.save(filePath, "JPEG", m_jpegQuality);
			m_playoutFrameIndex++;

			m_objectLock.lock();
		}
	}
}

// ----------------------------------------------------------------------------------------------------