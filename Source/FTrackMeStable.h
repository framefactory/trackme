// ----------------------------------------------------------------------------------------------------
//  Title			FTrackMeStable.h
//  Description		Precompiled headers for the TrackMe application
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRACKMESTABLE_H
#define FTRACKMESTABLE_H

// -- start of os-specific --
// Include first so Q_OS_XX is defined
#include <QtGlobal>

// OS-specific includes
#ifdef Q_OS_WIN
//#include <Windows.h>
#endif
// -- end of os-specific --

// Qt headers
#include <QtCore>
#include <QtGui>
//#include <QtNetwork>

// Other libraries
#include "Eigen/Dense"

#endif // FTRACKMESTABLE_H