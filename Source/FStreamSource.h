// ----------------------------------------------------------------------------------------------------
//  Title			FStreamSource.h
//  Description		Header file for FStreamSource.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 12 $
//  $Date: 2011-09-06 20:46:13 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMSOURCE_H
#define FSTREAMSOURCE_H

#include <QObject>
#include <QImage>
#include "FTrackMe.h"
#include "FPixelStruct.h"
#include "FGLTexture2D.h"
#include "FGLTextureRect.h"
#include "FVideoCaptureDeviceInfo.h"

class FVideoCaptureDevice;

// ----------------------------------------------------------------------------------------------------
//  Class FStreamSource
// ----------------------------------------------------------------------------------------------------

/// Reads from an image stream. This can be a live video source (webcam, etc.), a movie file
/// or a sequence of still images.
class FStreamSource : public QObject
{
	Q_OBJECT;

	//  Public types -----------------------------------------------------------

public:
	enum source_t
	{
		Image,
		ImageSequence,
		Movie,
		LiveVideo,
		DecklinkVideo
	};

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamSource(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamSource();

	//  Public commands --------------------------------------------------------

public:
	/// Opens a movie, a single image file or a sequence of image files.
	bool openFile(const QString& filePath);
	/// Opens the live stream with the given ordinal for capturing.
	bool openStream(size_t ordinal, const QSize& imageSize);
	/// Closes the current movie, sequence or live stream.
	void close();

	/// Advances to the next frame.
	void next();
	/// Advances to the previous frame.
	void previous();
	/// Moves to the begin of the movie/sequence.
	void toBegin();
	/// Moves to the end of the movie/sequence.
	void toEnd();
	/// Jumps to the given frame.
	void toPosition(size_t frame);

	/// Shows the configuration dialog for the current live stream source.
	void showStreamProperties();

	//  Public queries ---------------------------------------------------------

public:
	/// Copies the current frame to the given QImage.
	void getFrame(QImage& image);
	/// Copies the current frame to the given OpenGL 2D texture.
	void getFrame(FGLTexture2D& texture);
	/// Copies the current frame to the given OpenGL rectangle texture.
	void getFrame(FGLTextureRect& texture);
	/// Copies the current frame to the given buffer.
	void getFrame(void* pBuffer, size_t bufferSize);

	/// Returns true if a stream source has been opened.
	bool isOpen() const { return m_isOpen; }
	/// Returns true if a frame is available for reading.
	bool hasFrame();
	/// Returns true if a new frame is available for reading.
	/// This property returns false as soon as the new frame has been accessed for the first time.
	bool hasNewFrame();

	/// Returns the type of source of the stream.
	source_t sourceType() const { return m_sourceType; }
	/// Returns the (file-) name of the source.
	QString sourceName() const;
	/// Returns the file path of the source.
	const QString& sourcePath() const { return m_sourceName; }
	/// Returns the ordinal of the source stream.
	int streamOrdinal() const { return m_streamOrdinal; }

	/// Returns the total number of frames. Zero is returned for a live stream.
	size_t frameCount() const { return m_frameCount; }
	/// Returns the number of the current frame.
	size_t frameIndex() const { return m_frameIndex; }
	/// Returns the width and height of a frame.
	const QSize& frameSize() const { return m_frameSize; }
	/// Returns the frame size in bytes.
	size_t frameByteCount() const;

	/// Returns a list of video capture devices.
	QStringList captureDeviceList() const;

	//  Internal functions -----------------------------------------------------

private:
	QString _getFrameFileName(const QString& filePath, size_t index);
	bool _readImage(const QString& filePath);
	bool _initSequence(const QFileInfo& fileInfo);
	void _scanForDigits(const QString& filePath, int& firstPos, int& lastPos);
	bool _isImageFile(const QFileInfo& fileInfo);
	bool _isMovieFile(const QFileInfo& fileInfo);
	bool _readLiveFrame();

	//  Internal data members --------------------------------------------------

private:
	source_t m_sourceType;
	QString m_sourceName;
	int m_streamOrdinal;
	size_t m_frameCount;
	size_t m_frameIndex;
	size_t m_frameOffset;
	QSize m_frameSize;

	QList<FVideoCaptureDeviceInfo> m_captureDevices;
	FVideoCaptureDevice* m_pCaptureDevice;

	bool m_isOpen;
	bool m_hasFrame;
	bool m_hasNewFrame;

	QImage m_currentFrame;
	FPixelRGBA8u* m_pCurrentFrame;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMSOURCE_H