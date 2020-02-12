// ----------------------------------------------------------------------------------------------------
//  Title			FTrainingView.h
//  Description		Header file for FTrainingView.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 1 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FTRAININGVIEW_H
#define FTRAININGVIEW_H

#include "FTrackMe.h"
#include "FRenderScrollArea.h"
#include "FTrainingStatistics.h"

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingTextView
// ----------------------------------------------------------------------------------------------------

class FTrainingTextView : public QWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FTrainingTextView(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FTrainingTextView();

	//  Public commands --------------------------------------------------------

	void updateStatistics(const FTrainingStatistics& statistics);

	//  Overrides --------------------------------------------------------------

	virtual QSize sizeHint() const { return QSize(1024, 100); }
	virtual void paintEvent(QPaintEvent* pPaintEvent);

	//  Internal data members --------------------------------------------------

private:
	FTrainingStatistics m_statistics;
};

// ----------------------------------------------------------------------------------------------------
//  Class FTrainingView
// ----------------------------------------------------------------------------------------------------

class FTrainingView : public QWidget
{
	Q_OBJECT;

	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FTrainingView(QWidget* pParent = NULL);
	/// Virtual destructor.
	virtual ~FTrainingView();

	//  Public commands --------------------------------------------------------

public slots:
	void updateStatistics(FTrainingStatistics statistics);

	//  Public queries ---------------------------------------------------------

public:
	/// Returns the widget used for rendering OpenGL content.
	QWidget* renderWidget() const;

	//  Overrides --------------------------------------------------------------

	virtual QSize sizeHint() const { return QSize(1100, 900); }

	//  Internal functions -----------------------------------------------------

private:

	//  Internal data members --------------------------------------------------

private:
	FRenderScrollArea* m_pRenderArea;
	FTrainingTextView* m_pTextView;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FTRAININGVIEW_H