// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingView.cpp
//  Description		Implementation of class FTrainingView
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-17 00:16:08 +0200 (Mi, 17 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FTrainingView.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingTextView
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FTrainingTextView::FTrainingTextView(QWidget* pParent /* = NULL */)
: QWidget(pParent)
{
	m_statistics.clear();
}

FTrainingTextView::~FTrainingTextView()
{
}

// Public commands ------------------------------------------------------------------------------------

void FTrainingTextView::updateStatistics(const FTrainingStatistics& statistics)
{
	m_statistics = statistics;
	update();
}

// Overrides ------------------------------------------------------------------------------------------

void FTrainingTextView::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.fillRect(pPaintEvent->rect(), QColor(24, 24, 24));
	
	for (size_t i = 0; i < 8; i++)
	{
		FTrainingStatistics::contourInfo_t& info = m_statistics.contour[i];
		int x = i * 128 + 4;

		//painter.drawText(x, 12, QString("Contour %1").arg(i+1));
		if (info.isPresent)
		{
			painter.drawText(x, 12, QString("FA: %1, PA: %2")
				.arg(info.fittingAccuracy, 0, 'f', 2).arg(info.poseAmbiguity, 0, 'f', 2));
			painter.drawText(x, 28, QString("E: %1, SE: %2")
				.arg(info.meanSquaredError, 0, 'f', 2).arg(info.scaledMSE, 0, 'f', 2));
			
			if (info.isMatch)
			{
				painter.fillRect(x - 2, 33, 124, 14, QColor(0, 96, 48));
				painter.drawText(x, 44, "MATCH");
			}
			else
			{
				painter.fillRect(x - 2, 33, 124, 14, QColor(134, 96, 0));
				painter.drawText(x, 44, "INSERT");
			}

			/*
			if (info.isRecognized)
			{
				painter.fillRect(x - 2, 49, 124, 14, QColor(64, 128, 0));
				painter.drawText(x, 60, "CORRECT");
			}
			else
			{
				painter.fillRect(x - 2, 49, 124, 14, QColor(148, 32, 0));
				painter.drawText(x, 60, "WRONG");
			}
			*/

			painter.drawText(x, 60, QString("#Samples: %1").arg(info.sampleCount));
			painter.drawText(x, 76, QString("#classes: %1").arg(info.classCount));
		}
		else
		{
			painter.drawText(x, 28, QString("not present"));
		}
	}

	painter.fillRect(2, 81, 1020, 14, QColor(64, 64, 64));
	painter.drawText(4, 92, QString("Run: %1     Total Classes: %2     Total Samples: %3")
		.arg(m_statistics.runCount).arg(m_statistics.totalClassCount).arg(m_statistics.totalSampleCount));
	painter.drawText(350, 92, "FA: Fitting Accuracy, PA: Pose Ambiguity, E: Mean Squared Error, SE: Scaled MSE");
}

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingView
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FTrainingView::FTrainingView(QWidget* pParent /* = NULL */)
: QWidget(pParent)
{
	m_pRenderArea = new FRenderScrollArea(this);
	QVBoxLayout* pLayout = new QVBoxLayout(this);
	pLayout->setMargin(0);
	pLayout->setSpacing(0);
	pLayout->addWidget(m_pRenderArea);

	m_pRenderArea->setNativeResolution(FAspect(1024, 868));
	QWidget* pRenderWidget = m_pRenderArea->renderWidget();

	QVBoxLayout* pLayout2 = new QVBoxLayout(pRenderWidget);
	m_pTextView = new FTrainingTextView(pRenderWidget);
	pLayout2->setMargin(0);
	pLayout2->setSpacing(0);
	pLayout2->addStretch(2);
	pLayout2->addWidget(m_pTextView);
	pRenderWidget->setLayout(pLayout2);
}

FTrainingView::~FTrainingView()
{
}

// Public commands ------------------------------------------------------------------------------------

void FTrainingView::updateStatistics(FTrainingStatistics statistics)
{
	m_pTextView->updateStatistics(statistics);
}

// Public queries -------------------------------------------------------------------------------------

QWidget* FTrainingView::renderWidget() const
{
	return m_pRenderArea->renderWidget();
}

// Overrides ------------------------------------------------------------------------------------------

// Internal functions ---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------