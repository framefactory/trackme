// ----------------------------------------------------------------------------------------------------
//  Title			FDialogAbout.cpp
//  Description		Implementation of class FDialogAbout
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/30 $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "TrackMe.Version.h"
#include "FlowCore.Version.h"
#include "FlowUI.Version.h"
#include "FlowGL.Version.h"
#include "FlowMedia.Version.h"

#include "FDialogAbout.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FDialogAbout
// ----------------------------------------------------------------------------------------------------

// Static members -------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FDialogAbout::FDialogAbout(QWidget* pParent /* = NULL */)
: QWidget(pParent, Qt::Dialog)
{
	m_logo.load(":/images/trackme64");
	setWindowModality(Qt::WindowModal);
	setFixedSize(400, 300);
}

FDialogAbout::~FDialogAbout()
{
}

// Public commands ------------------------------------------------------------------------------------

// Public queries -------------------------------------------------------------------------------------

// Overrides ------------------------------------------------------------------------------------------

void FDialogAbout::paintEvent(QPaintEvent* pPaintEvent)
{
	QPainter painter(this);
	painter.drawImage(16, 16, m_logo);

	int x = 174;
	int x1 = 188;
	int y = 40;
	int dy = 16;

#ifdef QT_DEBUG
	QString config = "Debug";
#else
	QString config = "Release";
#endif

	QString versionText = QString("TrackMe - Version %1.%2 - %3 Build %4")
		.arg(FGlobalConstants::VERSION_MAJOR).arg(FGlobalConstants::VERSION_MINOR)
		.arg(config).arg(TRACKME_BUILD);

	painter.drawText(x, y - 8      , versionText);
	painter.drawText(x1, y + 1 * dy, "Developed by Ralph Wiedemeier");
	painter.drawText(x1, y + 2 * dy, "(c) 2011, all rights reserved");

	y = 120;
	painter.drawText(x, y - 8, "Libraries:");
	painter.drawText(x1, y + 1 * dy, QString("FlowCore Build %1").arg(FLOWCORE_BUILD));
	painter.drawText(x1, y + 2 * dy, QString("FlowUI Build %1").arg(FLOWUI_BUILD));
	painter.drawText(x1, y + 3 * dy, QString("FlowGL Build %1").arg(FLOWGL_BUILD));
	painter.drawText(x1, y + 4 * dy, QString("FlowMedia Build %1").arg(FLOWMEDIA_BUILD));
	painter.drawText(x1, y + 5 * dy, "Qt 4.7.3");
	painter.drawText(x1, y + 6 * dy, "Eigen 3.0.1");
	painter.drawText(x1, y + 7 * dy, "Glew 1.5.8");
	painter.drawText(x1, y + 8 * dy, "Assimp 2.0.863");
	painter.drawText(x1, y + 9 * dy, "levmar 2.5");
}

// Internal functions ---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------