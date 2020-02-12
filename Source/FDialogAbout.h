// ----------------------------------------------------------------------------------------------------
//  Title			FDialogAbout.h
//  Description		Header file for FDialogAbout.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: Ralph Wiedemeier $
//  $Revision: 1 $
//  $Date: 2011/08/30 $
// ----------------------------------------------------------------------------------------------------

#ifndef FDIALOGABOUT_H
#define FDIALOGABOUT_H

#include "FTrackMe.h"
#include <QWidget>

// ----------------------------------------------------------------------------------------------------
//  Class FDialogAbout
// ----------------------------------------------------------------------------------------------------

class FDialogAbout : public QWidget
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FDialogAbout(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FDialogAbout();

	//  Public commands --------------------------------------------------------

public:

	//  Public queries ---------------------------------------------------------


	//  Overrides --------------------------------------------------------------

protected:
	virtual void paintEvent(QPaintEvent* pPaintEvent);

	//  Internal functions -----------------------------------------------------

private:

	//  Internal data members --------------------------------------------------

private:
	QImage m_logo;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FDIALOGABOUT_H