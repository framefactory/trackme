// ----------------------------------------------------------------------------------------------------
//  Title			FStreamViewerFrame.cpp
//  Description		Implementation of class FStreamViewerFrame
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FRenderScrollArea.h"
#include "FRenderWidget.h"

#include "FStreamViewerFrame.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStreamViewerFrame
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStreamViewerFrame::FStreamViewerFrame(QWidget* pParent /* = NULL */)
: QWidget(pParent)
{
	m_pRenderArea = new FRenderScrollArea(this);
	m_pRenderArea->setNativeResolution(FAspect(768, 576));
	m_pRenderArea->setAllowEnlarge(false);
	m_pRenderArea->setAllowReduce(true);
	m_pRenderArea->setKeepAspect(true);

	_initialize();
	_initSpeedList();
	_initResolutionList();
	_initViewList();
}

FStreamViewerFrame::~FStreamViewerFrame()
{
}

// Public commands ------------------------------------------------------------------------------------

void FStreamViewerFrame::setMediaInfo(QString info)
{
	m_pLabelStatus->setText(info);
}

void FStreamViewerFrame::changeResolution(QSize resolution)
{
	if (!resolution.isEmpty())
	{
		FAspect res(resolution);
		m_pRenderArea->setNativeResolution(res);

		for (int i = 0; i < m_resolutionList.size(); i++)
		{
			if (res == m_resolutionList[i])
			{
				m_pComboResolution->setCurrentIndex(i);
				break;
			}
		}
	}
}

void FStreamViewerFrame::onComboResolution(int index)
{
	if (index >= 0 && index < m_resolutionList.size())
		m_pRenderArea->setNativeResolution(m_resolutionList[index]);
}

void FStreamViewerFrame::onComboSpeed(int index)
{
	if (index >= 0 && index < m_speedList.size())
		emit playbackSpeedChanged(m_speedList[index]);
}

void FStreamViewerFrame::onComboView(int index)
{
	if (index >= 0)
		emit viewModeChanged((quint32)index);
}

// Public queries -------------------------------------------------------------------------------------

FRenderWidget* FStreamViewerFrame::renderWidget() const
{
	return m_pRenderArea->renderWidget();
}

FRenderScrollArea* FStreamViewerFrame::renderScrollArea() const
{
	return m_pRenderArea;
}

// Overrides ------------------------------------------------------------------------------------------

// Internal functions ---------------------------------------------------------------------------------

void FStreamViewerFrame::_initialize()
{
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setMargin(0);
	pLayout->setSpacing(0);

	m_pToolbar = new QToolBar(this);

	m_pActPlay = m_pToolbar->addAction(QIcon(":/icons/media_play_16"), "Play",
		this, SIGNAL(buttonPlay()));
	m_pActStop = m_pToolbar->addAction(QIcon(":/icons/media_stop_16"), "Stop",
		this, SIGNAL(buttonStop()));

	m_pToolbar->addSeparator();
	
	m_pActPrev = m_pToolbar->addAction(QIcon(":/icons/media_previous_16"), "Previous Frame",
		this, SIGNAL(buttonPrevious()));
	m_pActNext = m_pToolbar->addAction(QIcon(":/icons/media_next_16"), "Next Frame",
		this, SIGNAL(buttonNext()));

	m_pToolbar->addSeparator();

	m_pActBegin = m_pToolbar->addAction(QIcon(":/icons/media_begin_16"), "Move To Begin",
		this, SIGNAL(buttonBegin()));
	m_pActEnd = m_pToolbar->addAction(QIcon(":/icons/media_end_16"), "Move To End",
		this, SIGNAL(buttonEnd()));

	m_pToolbar->addSeparator();

	m_pComboSpeed = new QComboBox(this);
	m_pComboSpeed->setMinimumWidth(90);
	m_pToolbar->addWidget(m_pComboSpeed);

	QLabel* pLabel = new QLabel("  ", this);
	pLabel->setAttribute(Qt::WA_NoSystemBackground);
	m_pToolbar->addWidget(pLabel);

	m_pComboResolution = new QComboBox(this);
	m_pComboResolution->setMinimumWidth(120);
	m_pToolbar->addWidget(m_pComboResolution);

	pLabel = new QLabel("  ", this);
	pLabel->setAttribute(Qt::WA_NoSystemBackground);
	m_pToolbar->addWidget(pLabel);

	m_pComboView = new QComboBox(this);
	m_pComboView->setMinimumWidth(120);
	m_pToolbar->addWidget(m_pComboView);

	m_pToolbar->addSeparator();

	m_pLabelStatus = new QLabel("", this);
	m_pLabelStatus->setAttribute(Qt::WA_NoSystemBackground);
	m_pLabelStatus->setWordWrap(false);
	m_pToolbar->addWidget(m_pLabelStatus);

	connect(m_pComboResolution, SIGNAL(currentIndexChanged(int)), SLOT(onComboResolution(int)));
	connect(m_pComboSpeed, SIGNAL(currentIndexChanged(int)), SLOT(onComboSpeed(int)));
	connect(m_pComboView, SIGNAL(currentIndexChanged(int)), SLOT(onComboView(int)));

	pLayout->addWidget(m_pToolbar);
	pLayout->addWidget(m_pRenderArea);

}

void FStreamViewerFrame::_initSpeedList()
{
	m_speedList.clear();

	m_speedList << 0.5f;
	m_speedList << 1.0f;
	m_speedList << 2.0f;
	m_speedList << 5.0f;
	m_speedList << 10.0f;
	m_speedList << 25.0f;
	m_speedList << 30.0f;
	m_speedList << 50.0f;

	m_pComboSpeed->clear();
	for (int i = 0; i < m_speedList.size(); i++)
		m_pComboSpeed->addItem(QString("%1 fps").arg(m_speedList[i]));
	m_pComboSpeed->setCurrentIndex(4);
}

void FStreamViewerFrame::_initResolutionList()
{
	m_resolutionList.clear();

	m_resolutionList << FAspect(384, 288);
	m_resolutionList << FAspect(640, 480);
	m_resolutionList << FAspect(768, 576);
	m_resolutionList << FAspect(768, 432);
	m_resolutionList << FAspect(800, 450);
	m_resolutionList << FAspect(1280, 720);
	m_resolutionList << FAspect(1920, 1080);

	m_pComboResolution->clear();
	for (int i = 0; i < m_resolutionList.size(); i++)
		m_pComboResolution->addItem(m_resolutionList[i].toString());
	m_pComboResolution->setCurrentIndex(2);
}

void FStreamViewerFrame::_initViewList()
{
	m_pComboView->clear();

	for (size_t i = 0; i < FStreamViewMode::optionCount; i++)
		m_pComboView->addItem(FStreamViewMode::optionAt(i).toString());
}

// ----------------------------------------------------------------------------------------------------