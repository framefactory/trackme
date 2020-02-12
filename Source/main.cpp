// ----------------------------------------------------------------------------------------------------
//  Title			main.cpp
//  Description		Main entry point of the application 
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 7 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FMainWindow.h"
#include "FMemoryTracer.h"

int main(int argc, char *argv[])
{
	MEMORY_TRACER_START;
	int retCode = 0;
	{
		QApplication application(argc, argv);
		application.setOrganizationName("ETH Zürich");
		application.setApplicationName("TrackMe");
		application.setApplicationVersion("0.1");

		QFile styleFile("flow_style.css");
		if (styleFile.open(QIODevice::ReadOnly | QIODevice::Text))
			application.setStyleSheet(styleFile.readAll());

		FMainWindow mainWindow;
		mainWindow.show();

		retCode = application.exec();
	}

	MEMORY_TRACER_REPORT;
	return retCode; 
}
