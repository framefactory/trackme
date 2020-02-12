// ----------------------------------------------------------------------------------------------------
//  Title			FParameterController.h
//  Description		Header file for FParameterController.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 13 $
//  $Date: 2011-08-25 21:02:54 +0200 (Do, 25 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FPARAMETERCONTROLLER_H
#define FPARAMETERCONTROLLER_H

#include "FTrackMe.h"
#include "FTreeWidget.h"
#include "FPTItems.h"
#include "FArchive.h"

class FStreamEngine;
class FStreamProcessor;

// ----------------------------------------------------------------------------------------------------
//  Class FParameterController
// ----------------------------------------------------------------------------------------------------

class FParameterController : public FTreeWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FParameterController(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FParameterController();

	//  Public commands --------------------------------------------------------

public:
	/// Sets the stream engine to be controlled.
	void setStreamEngine(FStreamEngine* pEngine);
	/// Sets the stream processor to be controlled.
	void setStreamProcessor(FStreamProcessor* pProcessor);

	/// Selects and activates a tracking preset.
	void activatePreset(int index);

	/// Writes a snapshot of all parameter values to the given archive.
	void write(FArchive& ar);
	/// Reads a previously stored parameter snapshot.
	void read(FArchive& ar);

protected slots:
	void selectModel(int index);
	void modelReset();


	//  Signals ----------------------------------------------------------------

signals:
	void mediaFileChanged(QString fileName);
	void modelChanged(int modelIndex);

	//  Internal functions -----------------------------------------------------

private:
	void _initialize();

	//  Internal data members --------------------------------------------------

	FStreamProcessor* m_pProcessor;
	FStreamEngine* m_pEngine;
	bool m_initialized;
	bool m_isReading;
	int m_modelIndex;

	FPTItemVector2* m_pCamRadialDist;
	FPTItemNumeric* m_pCamFocalDist;
	FPTItemVector2* m_pCamAperture;
	FPTItemVector3* m_pCamPosition;
	FPTItemVector3* m_pCamRotation;

	FPTItemVector3* m_pAugPosition;
	FPTItemVector3* m_pAugRotation;
	FPTItemNumeric* m_pAugScale;

	FPTItemSwitch* m_pEnableFocalDist;

	FPTItemFileSelect* m_pClassifierDataFile;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FPARAMETERCONTROLLER_H