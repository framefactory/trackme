// ----------------------------------------------------------------------------------------------------
//  Title			FStreamViewer.h
//  Description		Header file for FStreamViewer.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 13 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FSTREAMVIEWER_H
#define FSTREAMVIEWER_H

#include <QObject>
#include "FTrackMe.h"
#include "FlowGL.h"
#include "FMouseState.h"
#include "FTrackball.h"
#include "FStreamViewMode.h"

class FStreamEngine;

// ----------------------------------------------------------------------------------------------------
//  Class FStreamViewer
// ----------------------------------------------------------------------------------------------------

class FStreamViewer : public QObject
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FStreamViewer(const FGLContext& context, FStreamEngine* pEngine, QObject* pParent = NULL);
	/// Virtual destructor.
	virtual ~FStreamViewer();

	//  Public commands --------------------------------------------------------

public:
	/// Resets the stream viewer to the given frame size.
	void reset(const QSize& frameSize);
	/// Refreshes the viewer's content on the render window.
	void redraw();
	/// Clears the view area.
	void clear();
	/// Sets the view mode.
	void setMode(quint32 viewMode);
	/// Sets the size of the render window.
	void setWindowSize(const QSize& newSize);
		
	//  Public queries ---------------------------------------------------------

public:
	void readPixels(void* pData);
	const QSize& frameSize() const { return m_frameSize; }

	//  Internal functions -----------------------------------------------------

private:
	bool _initGL();
	bool _resetGL();

	//  Internal data members --------------------------------------------------

private:
	FGLContext m_context;
	FStreamEngine* m_pEngine;

	FStreamViewMode m_viewMode;
	QSize m_canvasSize;
	QSize m_frameSize;

	QRect m_viewportRect;

	FGLCanvas m_canvas;

	FGLProgram m_prgPlainImage;
	FGLProgram m_prgCanvasOverlay;
	FGLTextureRect m_sourceImage;
	FGLOverlayRect m_overlayRect;

	FGLProgram m_prgEdgeModel;
	FGLProgram m_prgViewHarris;
	FGLProgram m_prgDepthPass;
	FGLProgram m_prgViewDistanceTransform;
	FGLProgram m_prgViewContours;

	int m_uImageBlendFactor;
	int m_suSourceImage;
	int m_suOverlayImage;
	int m_suDepthImage;
	int m_suDTImage;
	int m_suContourData;

	int m_suHarrisSource;
	int m_suHarrisCorners;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FSTREAMVIEWER_H