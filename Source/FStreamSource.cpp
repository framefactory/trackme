// ----------------------------------------------------------------------------------------------------
//  Title			FStreamSource.cpp
//  Description		Implementation of class FStreamSource
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 12 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FCaptureManager.h"
#include "FVideoCaptureDevice.h"

#include "FStreamSource.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamSource
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamSource::FStreamSource(QObject* pParent /* = NULL */)
: QObject(pParent),
  m_sourceType(Image),
  m_streamOrdinal(-1),
  m_frameCount(0),
  m_frameIndex(0),
  m_frameOffset(0),
  m_frameSize(0, 0),
  m_isOpen(false),
  m_hasFrame(false),
  m_hasNewFrame(false),
  m_pCaptureDevice(NULL)
{
	FCaptureManager* pCapManager = FCaptureManager::instance();
	m_captureDevices = pCapManager->videoCaptureDeviceList();
}

FStreamSource::~FStreamSource()
{
	close();
}

// Public commands ------------------------------------------------------------------------------------

bool FStreamSource::openFile(const QString& filePath)
{
	close();

	m_sourceName = filePath;
	m_sourceName.replace(QChar('\\'), QChar('/'));

	QFileInfo info(m_sourceName);
	if (!info.exists())
	{
		m_sourceName.clear();
		return false;
	}

	if (_isImageFile(info))
	{
		QString firstFileName;
		if (_initSequence(info))
		{
			m_sourceType = ImageSequence;
			firstFileName = _getFrameFileName(m_sourceName, m_frameOffset);
		}
		else
		{
			m_sourceType = Image;
			m_frameCount = 1;
			firstFileName = m_sourceName;
		}

		if (!_readImage(firstFileName))
		{
			close();
			return false;
		}

		m_isOpen = true;
		return true;
	}
	else if (_isMovieFile(info))
	{
		F_ASSERT(!"Not yet implemented");
		return false;
	}

	F_ASSERT(!"FStreamSource::open - Neither an image nor a movie file");
	return false;
}

bool FStreamSource::openStream(size_t ordinal, const QSize& imageSize)
{
	close();

	F_ASSERT(ordinal < m_captureDevices.size());
	if (ordinal >= m_captureDevices.size())
		return false;

	m_streamOrdinal = ordinal;

	m_pCaptureDevice = FCaptureManager::instance()->createDevice(m_captureDevices[ordinal], NULL);
	if (!m_pCaptureDevice)
		return false;

	m_frameSize = imageSize;
	m_pCaptureDevice->setFrameSize(m_frameSize);
	m_pCaptureDevice->setFrameFormat(FVideoFormat::RGB24);

	bool success = m_pCaptureDevice->openDevice();
	if (!success)
	{
		F_SAFE_DELETE(m_pCaptureDevice);
		return false;
	}

	success = m_pCaptureDevice->startStreaming();
	if (!success)
	{
		m_pCaptureDevice->closeDevice();
		F_SAFE_DELETE(m_pCaptureDevice);
		return false;
	}

	m_currentFrame = QImage(m_frameSize, QImage::Format_ARGB32);
	m_sourceName = m_pCaptureDevice->name();
	m_sourceType = LiveVideo;
	m_isOpen = true;

	return true;
}

void FStreamSource::close()
{
	if (m_sourceType == Movie)
	{
	}
	else if (m_sourceType == LiveVideo)
	{
		if (m_pCaptureDevice)
		{
			m_pCaptureDevice->stopStreaming();
			m_pCaptureDevice->closeDevice();
			F_SAFE_DELETE(m_pCaptureDevice);
		}
	}
	else if (m_sourceType == DecklinkVideo)
	{
	}

	m_currentFrame = QImage();
	m_sourceName.clear();
	m_sourceType = Image;
	m_streamOrdinal = -1;
	m_frameCount = 0;
	m_frameIndex = 0;
	m_frameOffset = 0;
	m_frameSize = QSize(0, 0);
	m_isOpen = false;
	m_hasFrame = false;
	m_hasNewFrame = false;
}

void FStreamSource::next()
{
	F_ASSERT(m_isOpen);

	if (m_sourceType == LiveVideo || m_sourceType == DecklinkVideo)
	{
		_readLiveFrame();
		return;
	}

	if (m_sourceType != Movie && m_sourceType != ImageSequence)
		return;

	if (m_frameIndex < m_frameCount - 1)
	{
		m_frameIndex++;
		if (m_sourceType == ImageSequence)
		{
			_readImage(_getFrameFileName(m_sourceName, m_frameIndex + m_frameOffset));
			return;
		}
		else if (m_sourceType == Movie)
		{
			F_ASSERT(false); // TODO: Implement retrieve next movie frame
		}
	}
}

void FStreamSource::previous()
{
	F_ASSERT(m_isOpen);
	if (m_sourceType != Movie && m_sourceType != ImageSequence)
		return;

	if (m_frameIndex > 0)
	{
		m_frameIndex--;
		if (m_sourceType == ImageSequence)
		{
			_readImage(_getFrameFileName(m_sourceName, m_frameIndex + m_frameOffset));
			return;
		}
		else if (m_sourceType == LiveVideo)
		{
			_readLiveFrame();
		}
	}
}

void FStreamSource::toBegin()
{
	F_ASSERT(m_isOpen);
	if (m_sourceType != Movie && m_sourceType != ImageSequence)
		return;

	m_frameIndex = 0;
	if (m_sourceType == ImageSequence)
	{
		_readImage(_getFrameFileName(m_sourceName, m_frameIndex + m_frameOffset));
		return;
	}
}

void FStreamSource::toEnd()
{
	F_ASSERT(m_isOpen);
	if (m_sourceType != Movie && m_sourceType != ImageSequence)
		return;

	m_frameIndex = m_frameCount - 1;
	if (m_sourceType == ImageSequence)
	{
		_readImage(_getFrameFileName(m_sourceName, m_frameIndex + m_frameOffset));
		return;
	}
}

void FStreamSource::toPosition(size_t frame)
{
	F_ASSERT(m_isOpen);
	if (m_sourceType != Movie && m_sourceType != ImageSequence)
		return;

	m_frameIndex = fMinMax(frame, (size_t)0, m_frameCount - 1);
	if (m_sourceType == ImageSequence)
	{
		_readImage(_getFrameFileName(m_sourceName, m_frameIndex + m_frameOffset));
		return;
	}
}

void FStreamSource::showStreamProperties()
{
	if (m_streamOrdinal > -1)
	{
		m_pCaptureDevice->showPropertyPages();
	}
}

// Public queries -------------------------------------------------------------------------------------

void FStreamSource::getFrame(QImage& image)
{
	F_ASSERT(m_hasFrame && !m_currentFrame.isNull());
	if (!m_hasFrame)
		return;

	image = m_currentFrame;
	m_hasNewFrame = false;
}

void FStreamSource::getFrame(FGLTexture2D& texture)
{
	F_ASSERT(texture.isValid());
	F_ASSERT(m_hasFrame && !m_currentFrame.isNull());
	if (!m_hasFrame)
		return;

	texture.loadFromImage(m_currentFrame);
	m_hasNewFrame = false;
}

void FStreamSource::getFrame(FGLTextureRect& texture)
{
	F_ASSERT(texture.isValid());
	F_ASSERT(m_hasFrame && !m_currentFrame.isNull());
	if (!m_hasFrame)
		return;

	texture.loadFromImage(m_currentFrame);
	m_hasNewFrame = false;
}

void FStreamSource::getFrame(void* pBuffer, size_t bufferSize)
{
	F_ASSERT(m_hasFrame && !m_currentFrame.isNull());
	if (!m_hasFrame)
		return;

	size_t bytes = fMin(bufferSize, (size_t)m_currentFrame.byteCount());
	memcpy(pBuffer, m_currentFrame.bits(), bytes);
	m_hasNewFrame = false;
}

QString FStreamSource::sourceName() const
{
	int lastSlash = m_sourceName.lastIndexOf('/');
	if (lastSlash >= 0)
		return m_sourceName.mid(lastSlash + 1);
	else
		return m_sourceName;
}

QStringList FStreamSource::captureDeviceList() const
{
	QStringList deviceNames;
	for (int i = 0; i < m_captureDevices.size(); i++)
		deviceNames << m_captureDevices[i].toString();
	return deviceNames;
}

bool FStreamSource::hasFrame()
{
	if (!m_isOpen)
		return false;

	return m_hasFrame;
}

bool FStreamSource::hasNewFrame()
{
	if (!m_isOpen)
		return false;

	return m_hasNewFrame;
}

// Internal functions ---------------------------------------------------------------------------------

QString FStreamSource::_getFrameFileName(const QString& filePath, size_t index)
{
	int firstPos, lastPos;
	_scanForDigits(filePath, firstPos, lastPos);
	int numDigits = lastPos - firstPos + 1;

	QString textIndex;
	if (numDigits > 0)
		textIndex = QString("%1").arg((ulong)index, numDigits, 10, QLatin1Char('0'));

	return QString("%1%2%3").arg(filePath.left(firstPos))
		.arg(textIndex).arg(filePath.mid(lastPos + 1));
}

bool FStreamSource::_readImage(const QString& filePath)
{
	//F_TRACE(QString("FStreamSource::_readImage - Reading file: %1").arg(filePath));

	m_hasFrame = m_hasNewFrame = false;
	m_currentFrame.load(filePath);
	
	if (m_currentFrame.isNull())
		return false;

	if (m_currentFrame.format() != QImage::Format_ARGB32)
		m_currentFrame = m_currentFrame.convertToFormat(QImage::Format_ARGB32);

	if (m_currentFrame.isNull())
		return false;

	if (!m_frameSize.isNull())
	{
		if (m_frameSize != m_currentFrame.size())
		{
			m_currentFrame = QImage();
			return false;
		}
	}
	else
	{
		m_frameSize = m_currentFrame.size();
	}

	m_hasFrame = m_hasNewFrame = true;
	return true;
}

bool FStreamSource::_initSequence(const QFileInfo& fileInfo)
{
	int firstPos, lastPos;
	QString fileName = fileInfo.fileName();
	_scanForDigits(fileName, firstPos, lastPos);
	if (firstPos == -1)
		return false;

	int numDigits = lastPos - firstPos + 1;
	QString regText = fileName.left(firstPos)
		+ QString("\\d{%1,%2}").arg(numDigits).arg(numDigits)
		+ fileName.mid(lastPos + 1);
	QRegExp regEx(regText, Qt::CaseInsensitive);

	QDirIterator dirIter(fileInfo.path());
	int minIndex = INT_MAX;
	int maxIndex = -1;

	while(dirIter.hasNext())
	{
		dirIter.next();
		QString fileName = dirIter.fileName();
		if (regEx.exactMatch(fileName))
		{
			int firstPos, lastPos;
			bool isOk = false;
			_scanForDigits(fileName, firstPos, lastPos);
			int index = fileName.mid(firstPos, lastPos - firstPos + 1).toInt(&isOk);
			if (isOk)
			{
				minIndex = fMin(minIndex, index);
				maxIndex = fMax(maxIndex, index);
			}
		}
	}
	
	if (maxIndex > minIndex && minIndex >= 0)
	{
		m_frameOffset = minIndex;
		m_frameCount = maxIndex - minIndex + 1;
		F_TRACE(QString("FStreamSource::_initSequence - Offset: %1, Count: %2")
			.arg(m_frameOffset).arg(m_frameCount));
		return true;
	}

	return false;
}

void FStreamSource::_scanForDigits(const QString& filePath, int& firstPos, int& lastPos)
{
	int dotPos = filePath.lastIndexOf('.');
	lastPos = dotPos - 1;
	while(lastPos >= 0 && !filePath[lastPos].isDigit())
		lastPos--;
	firstPos = lastPos;
	while(firstPos >= 0 && filePath[firstPos].isDigit())
		firstPos--;

	if (lastPos >= 0)
		firstPos++;
}

bool FStreamSource::_isImageFile(const QFileInfo& fileInfo)
{
	QString ext = fileInfo.suffix();
	return (ext.compare("tif", Qt::CaseInsensitive) == 0
		|| ext.compare("jpg", Qt::CaseInsensitive) == 0
		|| ext.compare("png", Qt::CaseInsensitive) == 0
		|| ext.compare("bmp", Qt::CaseInsensitive) == 0);
}

bool FStreamSource::_isMovieFile(const QFileInfo& fileInfo)
{
	QString ext = fileInfo.suffix();
	return (ext.compare("avi", Qt::CaseInsensitive) == 0
		|| ext.compare("mp4", Qt::CaseInsensitive) == 0
		|| ext.compare("mov", Qt::CaseInsensitive) == 0);
}

bool FStreamSource::_readLiveFrame()
{
	F_ASSERT(m_isOpen);
	if (!m_isOpen)
		return false;

	F_ASSERT(m_pCaptureDevice && m_pCaptureDevice->isStreaming());

	bool result = m_pCaptureDevice->waitForFrame(2000);
	if (!result)
		fWarning("Stream Source", "Timeout while waiting for new frame");

	m_pCaptureDevice->readFrame(m_currentFrame.bits(), m_currentFrame.byteCount());

	m_hasFrame = true;
	m_hasNewFrame = true;

	return true;
}

// ----------------------------------------------------------------------------------------------------