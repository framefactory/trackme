// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingParameterController.h
//  Description		Header file for FTrainingParameterController.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-09-04 18:33:19 +0200 (So, 04 Sep 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGPARAMETERCONTROLLER_H
#define FTRAININGPARAMETERCONTROLLER_H

#include "FTrackMe.h"
#include "FTreeWidget.h"
#include "FPTItems.h"
#include "FVectorT.h"
#include "FArchive.h"

class FTrainingEngine;

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingParameterController
// ----------------------------------------------------------------------------------------------------

/// Tree of parameter widgets for controlling the contour training.
class FTrainingParameterController : public FTreeWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FTrainingParameterController(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FTrainingParameterController();

	//  Public commands --------------------------------------------------------

	/// Sets the training engine and initializes the parameter tree.
	void setEngine(FTrainingEngine* pEngine);

	/// Writes a snapshot of all parameter values to the given archive.
	void write(FArchive& ar);
	/// Reads a previously stored parameter snapshot.
	void read(FArchive& ar);

	//  Public queries ---------------------------------------------------------

	//  Overrides --------------------------------------------------------------

	virtual QSize sizeHint() const { return QSize(320, 640); }

	//  Internal functions -----------------------------------------------------

private:
	void _initialize();

	//  Internal data members --------------------------------------------------

private:
	bool m_isInitialized;
	FTrainingEngine* m_pEngine;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGPARAMETERCONTROLLER_H