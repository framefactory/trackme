// ----------------------------------------------------------------------------------------------------
//  Title			FStreamProcessor.cpp
//  Description		Implementation of class FStreamProcessor
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 15 $
//  $Date: 2011-09-15 13:32:33 +0200 (Do, 15 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FStreamEngine.h"
#include "FStreamSource.h"
#include "FStreamViewer.h"
#include "FRenderWidget.h"

#include "FGenericModel.h"
#include "FBoxModel.h"
#include "FStudioModelVirtEco.h"

#include "FStreamProcessor.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamProcessor
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamProcessor::FStreamProcessor(QObject* pParent /* = NULL */)
: QObject(pParent),
  m_pSource(NULL),
  m_pEngine(NULL),
  m_pViewer(NULL),
  m_pRenderWidget(NULL),
  m_isInitialized(false),
  m_mediaState(NoSource),
  m_frameDuration(0.1),
  m_playoutEnabled(false)
{
}

FStreamProcessor::~FStreamProcessor()
{
	F_SAFE_DELETE(m_pSource);
	F_SAFE_DELETE(m_pEngine);
	F_SAFE_DELETE(m_pViewer);
}

// Public commands ------------------------------------------------------------------------------------

bool FStreamProcessor::process()
{
	bool needRedraw = false;

	if ((m_mediaState == Playing && m_timer.isElapsed()) || m_mediaState == Streaming)
	{
		if (m_pSource->isOpen())
		{
			m_timer.start(m_frameDuration);
			m_pSource->next();
			if (!m_pSource->hasNewFrame())
			{
				m_pSource->toBegin();
				m_pEngine->resetPose();
				needRedraw = true;
			}
			_changeMediaState(Playing);
		}
		else
		{
			_changeMediaState(NoSource);
		}
	}

	if (m_pSource->hasNewFrame()
		|| (m_pSource->isOpen() && m_pEngine->wantRedraw()))
	{
		m_pSource->getFrame(m_sourceFrame);
		if (m_sourceFrame.isValid())
		{
			m_statistics.frameNumber = m_pSource->frameIndex();
			m_pEngine->processFrame(m_sourceFrame, &m_statistics);
			needRedraw = true;
		}
	}

	if (m_mediaState != NoSource)
	{
		if (needRedraw)
		{
			emit statisticsUpdated(m_statistics);
			m_pViewer->redraw();

			if (m_playoutEnabled)
				m_playoutEngine.writeBackbuffer();
		}
	}
	else
	{
		//m_pViewer->clear();
	}

	return true; // keep running
}

bool FStreamProcessor::initialize(FRenderWidget* pRenderWidget)
{
	F_ASSERT(pRenderWidget);
	m_pRenderWidget = pRenderWidget;

	FGLContextSettings settings;
	settings.setSampleCount(16);
	settings.setVersion(3, 3);

	if (!m_context.create(settings, pRenderWidget))
		return false;

	m_pSource = new FStreamSource(this);
	m_pEngine = new FStreamEngine(this);
	m_pViewer = new FStreamViewer(m_context, m_pEngine, this);

	connect(m_pRenderWidget, SIGNAL(keyStateChanged(FKeyboardState)),
		this, SLOT(keyStateChanged(FKeyboardState)));
	connect(m_pRenderWidget, SIGNAL(mouseStateChanged(FMouseState)),
		this, SLOT(mouseStateChanged(FMouseState)), Qt::DirectConnection);
	connect(m_pRenderWidget, SIGNAL(sizeChanged(QSize)),
		this, SLOT(windowSizeChanged(QSize)));

	m_sourceFrame.create();

	/*
	// Create the line model used for line tracking
	FBoxModel* pModel = new FBoxModel();
	pModel->setSize(100.0f, 20.0f, 100.0f);
	pModel->create();
	F_ASSERT(pModel->isValid());

	m_pLineModel = pModel;
	m_pEngine->setLineModel(m_pLineModel);
	*/

	m_pEngine->reset(QSize(768, 576));
	m_pViewer->reset(QSize(768, 576));

	m_isInitialized = true;
	return true;
}

// Public slots ---------------------------------------------------------------------------------------

/*
void FStreamProcessor::selectModel(int modelIndex)
{
	F_SAFE_DELETE(m_pLineModel);

	switch (modelIndex)
	{
	case 0: // Simple Cube
		{
			FBoxModel* pModel = new FBoxModel();
			pModel->setSize(100.0f, 20.0f, 100.0f);
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 1: // SF Cube
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/cgCubeSF_solid.dae");
			pModel->setEdgeModelFile("Models/cgCubeSF_edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 2: // SF Anchor Box
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/Anchorbox SF solid.dae");
			pModel->setEdgeModelFile("Models/Anchorbox SF edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 3: // ECO Set plain
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/ECO Set solid.dae");
			pModel->setEdgeModelFile("Models/ECO Set edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 4: // ECO Set with Table plain
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/ECO Set Table solid 2.dae");
			pModel->setEdgeModelFile("Models/ECO Set Table edges 2.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 5: // ECO Set with Logo on floor
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/ECO Set solid.dae");
			pModel->setEdgeModelFile("Models/ECO Set edges logo on floor.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 6: // ECO Set with Logo on table
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/ECO Set Table solid.dae");
			pModel->setEdgeModelFile("Models/ECO Set Table edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 7: // ECO Virtual set
		{
			FStudioModelVirtEco* pModel = new FStudioModelVirtEco();
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 8: // Anchor Bench
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/AnchorBench.dae");
			pModel->setEdgeModelFile("Models/AnchorBench ECO edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	case 9: // SF Meteo
		{
			FGenericModel* pModel = new FGenericModel();
			pModel->setSolidModelFile("Models/Meteo Set solid.dae");
			pModel->setEdgeModelFile("Models/Meteo Set edges.dae");
			pModel->create();
			F_ASSERT(pModel->isValid());
			m_pLineModel = pModel;
		}
		break;
	default:
		F_ASSERT(false);
		break;
	}

	m_pEngine->setLineModel(m_pLineModel);
	//m_pEngine->reset(m_pSource->frameSize());
}
*/

void FStreamProcessor::openMediaFile(QString filePath)
{
	F_TRACE("FStreamProcessor::openMediaFile");
	m_pSource->openFile(filePath);

	if (!m_pSource->isOpen())
	{
		_changeMediaState(NoSource);
		return;
	}

	_changeMediaState(Stopped);

	// Get new frame size and reset engine and viewer
	QSize frameSize = m_pSource->frameSize();
	m_pEngine->reset(frameSize);
	m_pViewer->reset(frameSize);
	emit frameSizeChanged(frameSize);
}

void FStreamProcessor::openMediaStream(int ordinal, QSize imageSize)
{
	F_TRACE("FStreamProcessor::openMediaStream");
	m_pSource->openStream(ordinal, imageSize);

	if (!m_pSource->isOpen())
	{
		_changeMediaState(NoSource);
		return;
	}

	QSize frameSize = m_pSource->frameSize();
	if (frameSize.isEmpty())
	{
		_changeMediaState(NoSource);
		return;
	}

	_changeMediaState(Streaming);
	m_pEngine->reset(frameSize);
	m_pViewer->reset(frameSize);
	emit frameSizeChanged(frameSize);
}

void FStreamProcessor::showStreamProperties()
{
	if (m_pSource->isOpen() && m_pSource->sourceType() == FStreamSource::LiveVideo)
		m_pSource->showStreamProperties();
}

void FStreamProcessor::playMedia()
{
	if (m_pSource->isOpen() && m_mediaState == Stopped)
	{
		_changeMediaState(Playing);
		m_timer.start(0.1);
	}
}

void FStreamProcessor::stopMedia()
{
	if (m_mediaState == Playing)
	{
		_changeMediaState(Stopped);
	}
}

void FStreamProcessor::previousFrame()
{
	if (m_pSource->isOpen())
	{
		m_pSource->previous();
		_changeMediaState(m_mediaState);
	}
}

void FStreamProcessor::nextFrame()
{
	if (m_pSource->isOpen())
	{
		m_pSource->next();
		_changeMediaState(m_mediaState);
	}
}

void FStreamProcessor::firstFrame()
{
	if (m_pSource->isOpen())
	{
		m_pSource->toBegin();
		_changeMediaState(m_mediaState);
		m_pEngine->resetPose();
	}
}

void FStreamProcessor::lastFrame()
{
	if (m_pSource->isOpen())
	{
		m_pSource->toEnd();
		_changeMediaState(m_mediaState);
	}
}

void FStreamProcessor::setPlaybackSpeed(float fps)
{
	F_ASSERT(fps > 0.0f);
	m_frameDuration = 1.0 / fps;
}

void FStreamProcessor::setViewMode(quint32 mode)
{
	m_pViewer->setMode(mode);
	m_pViewer->redraw();
}

void FStreamProcessor::startPlayout(QString filePath)
{
	if (m_playoutEnabled)
		return;

	int dotPos = filePath.indexOf('.');
	QString playoutFilePath;

	if (dotPos >= 0)
		playoutFilePath = filePath.left(dotPos);
	else
		playoutFilePath = filePath;

	m_playoutEngine.start(playoutFilePath, m_pViewer->frameSize());
	m_playoutEnabled = true;
}

void FStreamProcessor::stopPlayout()
{
	if (!m_playoutEnabled)
		return;

	m_playoutEngine.stop();
	m_playoutEnabled = false;
}

void FStreamProcessor::keyStateChanged(FKeyboardState keyState)
{
	if (keyState.eventType() == QEvent::KeyPress)
	{
	}
}

void FStreamProcessor::mouseStateChanged(FMouseState mouseState)
{
}

void FStreamProcessor::windowSizeChanged(QSize newSize)
{
	m_pViewer->setWindowSize(newSize);

	if (m_pSource->isOpen())
		m_pViewer->redraw();
}

// Internal functions ---------------------------------------------------------------------------------

void FStreamProcessor::_changeMediaState(mediaState_t state)
{
	m_mediaState = state;
	QString info;

	if (state == NoSource)
	{
		info = "";
	}
	else if (m_pSource->isOpen())
	{
		info = QString("%1\nFrame %2 of %3")
			.arg(m_pSource->sourceName())
			.arg(m_pSource->frameIndex() + 1)
			.arg(m_pSource->frameCount());
	}
	else
		info = QString("Error while opening/reading media\n%1").arg(m_pSource->sourceName());

	emit mediaStateChanged(info);
}

// ----------------------------------------------------------------------------------------------------