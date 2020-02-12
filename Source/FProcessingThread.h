// ----------------------------------------------------------------------------------------------------
//  Title			FProcessingThread.h
//  Description		Header file for FProcessingThread.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FPROCESSINGTHREAD_H
#define FPROCESSINGTHREAD_H

#include <QThread>
#include "FTrackMe.h"

class FStreamProcessor;
class FStreamSource;
class FStreamEngine;
class FStreamViewer;
class FRenderWidget;

// ----------------------------------------------------------------------------------------------------
//  Class FProcessingThread
// ----------------------------------------------------------------------------------------------------

class FProcessingThread : public QThread
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FProcessingThread(QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FProcessingThread();

	//  Public commands --------------------------------------------------------

public:
	/// Starts the thread.
	void start(QThread::Priority priority = QThread::NormalPriority);
	/// Stops the thread.
	void stop();

	/// Sets the widget to be used for rendering.
	void setRenderWidget(FRenderWidget* pRenderWidget);

	//  Public queries ---------------------------------------------------------

	/// Returns true if the thread is running and is initialized.
	bool isInitialized() const { return m_isInitialized; }

	/// Returns the stream processor.
	FStreamProcessor* streamProcessor() const { return m_pProcessor; }
	/// Returns the stream source.
	FStreamSource* streamSource() const;
	/// Returns the stream engine.
	FStreamEngine* streamEngine() const;
	/// Returns the stream viewer.
	FStreamViewer* streamViewer() const;

	//  Overrides --------------------------------------------------------------

protected:
	virtual void run();

	//  Internal functions -----------------------------------------------------

private:

	//  Internal data members --------------------------------------------------

private:
	bool m_wantExit;
	bool m_isInitialized;
	FRenderWidget* m_pRenderWidget;
	FStreamProcessor* m_pProcessor;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FPROCESSINGTHREAD_H