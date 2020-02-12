// ----------------------------------------------------------------------------------------------------
//  Title			FStreamPlayout.h
//  Description		Header file for FStreamPlayout.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/31 $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMPLAYOUT_H
#define FSTREAMPLAYOUT_H

#include <vector>
#include <QThread>
#include <QImage>
#include "FTrackMe.h"
#include "FGLTextureRect.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamPlayout
// ----------------------------------------------------------------------------------------------------

class FStreamPlayout : public QThread
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamPlayout(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamPlayout();

	//  Public commands --------------------------------------------------------

public:
	void start(const QString& filePath, const QSize& frameSize);
	void stop();

	void writeFrame(const FGLTextureRect& frame);
	void writeBackbuffer();

	//  Public queries ---------------------------------------------------------


	//  Overrides --------------------------------------------------------------

protected:
	virtual void run();

	//  Internal functions -----------------------------------------------------

private:

	//  Internal data members --------------------------------------------------

private:
	QSize m_frameSize;
	QString m_baseFilePath;
	std::vector<QImage> m_frameQueue;
	QImage m_frameBuffer;
	size_t m_readIndex;
	size_t m_writeIndex;
	size_t m_playoutFrameIndex;

	int m_jpegQuality;

	bool m_wantExit;
	QMutex m_objectLock;
	QWaitCondition m_waitForFrame;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMPLAYOUT_H