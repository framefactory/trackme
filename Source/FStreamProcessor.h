// ----------------------------------------------------------------------------------------------------
//  Title			FStreamProcessor.h
//  Description		Header file for FStreamProcessor.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-06 22:24:21 +0200 (Di, 06 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMPROCESSOR_H
#define FSTREAMPROCESSOR_H

#include <QObject>
#include "FTrackMe.h"
#include "FlowGL.h"
#include "FTimer.h"

#include "FStreamPlayout.h"
#include "FFrameStatistics.h"
#include "FKeyboardState.h"
#include "FMouseState.h"

class FStreamEngine;
class FStreamSource;
class FStreamViewer;
class FRenderWidget;
class FLineModel;

// ----------------------------------------------------------------------------------------------------
//  Class FStreamProcessor
// ----------------------------------------------------------------------------------------------------

class  FStreamProcessor : public QObject
{
	Q_OBJECT;

	//  Public types -----------------------------------------------------------

public:
	enum mediaState_t
	{
		NoSource,
		Stopped,
		Playing,
		Streaming
	};

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamProcessor(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamProcessor();

	//  Public commands --------------------------------------------------------

public:
	/// Initialization of the engine.
	bool initialize(FRenderWidget* pRenderWidget);
	/// Processes one step.
	bool process();

public slots:
	void openMediaFile(QString filePath);
	void openMediaStream(int ordinal, QSize imageSize);
	void showStreamProperties();
	
	void playMedia();
	void stopMedia();
	void previousFrame();
	void nextFrame();
	void firstFrame();
	void lastFrame();

	void setPlaybackSpeed(float fps);
	void setViewMode(quint32 mode);

	void startPlayout(QString filePath);
	void stopPlayout();

	void keyStateChanged(FKeyboardState keyState);
	void mouseStateChanged(FMouseState mouseState);
	void windowSizeChanged(QSize newSize);


	//  Public queries ---------------------------------------------------------

public:
	/// Returns true if the processor was initialized successfully.
	bool isInitialized() const { return m_isInitialized; }

	/// Returns the stream source.
	FStreamSource* streamSource() const { return m_pSource; }
	/// Returns the stream engine.
	FStreamEngine* streamEngine() const { return m_pEngine; }
	/// Returns the stream viewer.
	FStreamViewer* streamViewer() const { return m_pViewer; }

	//  Signals ----------------------------------------------------------------

signals:
	void mediaStateChanged(QString mediaStateInfo);
	void statisticsUpdated(FFrameStatistics stats);
	void frameSizeChanged(QSize frameSize);

	//  Internal functions -----------------------------------------------------

private:
	void _changeMediaState(mediaState_t state);

	//  Internal data members --------------------------------------------------

private:
	bool m_isInitialized;
	mediaState_t m_mediaState;
	double m_frameDuration;

	FGLContext m_context;
	FTimer m_timer;

	FStreamSource* m_pSource;
	FStreamEngine* m_pEngine;
	FStreamViewer* m_pViewer;
	FRenderWidget* m_pRenderWidget;

	FGLTextureRect m_sourceFrame;	

	FStreamPlayout m_playoutEngine;
	bool m_playoutEnabled;

	FFrameStatistics m_statistics;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMPROCESSOR_H