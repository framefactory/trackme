// ----------------------------------------------------------------------------------------------------
//  Title			FStudioModelVirtEco.cpp
//  Description		Implementation of class FStudioModelVirtEco
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 12 $
//  $Date: 2011-08-30 20:28:52 +0200 (Di, 30 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FMultiMesh.h"

#include "FStudioModelVirtEco.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FStudioModelVirtEco
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FStudioModelVirtEco::FStudioModelVirtEco()
//: m_offset(-200.0f, 106.0f, -206.0f)
: m_offset(-200.0f, 0.0f, -206.0f)
{
}

FStudioModelVirtEco::~FStudioModelVirtEco()
{
}

// Public commands ------------------------------------------------------------------------------------

void FStudioModelVirtEco::setOffset(const FVector3f& offset)
{
	m_offset = offset;
}

// Overrides ------------------------------------------------------------------------------------------

void FStudioModelVirtEco::onCreateSolidMesh(FGLMesh& solidMesh)
{
	FMultiMesh m_boxes;

	// Base including lower wood shelf
	//m_boxes.createBox(FVector3f(  0.0f, -45.0f, 100.0f), FVector3f(200.0f, 0.0f, 150.0f));
	m_boxes.createBox(FVector3f(  0.0f,   0.0f, -150.0f), FVector3f(200.0f,  45.0f, -100.0f));
	// Left wood shelf
	//m_boxes.createBox(FVector3f(  0.0f, -95.0f, 100.0f), FVector3f(  5.0f, -45.0f, 150.0f));
	m_boxes.createBox(FVector3f(  0.0f,  45.0f, -150.0f), FVector3f(  5.0f,  95.0f, -100.0f));
	// Right wood shelf
	//m_boxes.createBox(FVector3f(195.0f, -95.0f, 100.0f), FVector3f(200.0f, -45.0f, 150.0f));
	m_boxes.createBox(FVector3f(195.0f,  45.0f, -150.0f), FVector3f(200.0f,  95.0f, -100.0f));
	// Top wood shelf
	//m_boxes.createBox(FVector3f(  0.0f, -100.0f, 100.0f), FVector3f(200.0f, -95.0f, 150.0f));
	m_boxes.createBox(FVector3f(  0.0f,  95.0f, -150.0f), FVector3f(200.0f, 100.0f, -100.0f));

	// Add offset
	for (size_t i = 0, n = m_boxes.vertices().size(); i < n; i++)
		m_boxes.vertices()[i].pos += m_offset;

	// Create OpenGL mesh object
	solidMesh.create(m_boxes);
}

void FStudioModelVirtEco::onCreateEdges(edgeVec_t& edges)
{
	edges.push_back(edge_t(  0.0f,   0.0f, -100.0f, 200.0f,   0.0f, -100.0f)); // Line  0 (Pt  0 -  1)
	edges.push_back(edge_t(200.0f,   0.0f, -100.0f, 200.0f,   0.0f, -150.0f)); // Line  1 (Pt  1 -  2)
	edges.push_back(edge_t(  0.0f,  40.0f, -100.0f, 200.0f,  40.0f, -100.0f)); // Line  2 (Pt  3 -  4)
	edges.push_back(edge_t(200.0f,  40.0f, -100.0f, 200.0f,  40.0f, -150.0f)); // Line  3 (Pt  4 -  5)
	edges.push_back(edge_t(  0.0f,  40.0f, -100.0f,   0.0f, 100.0f, -100.0f)); // Line  4 (Pt  3 -  7)
	edges.push_back(edge_t(200.0f,  40.0f, -100.0f, 200.0f, 100.0f, -100.0f)); // Line  5 (Pt  4 -  8)
	edges.push_back(edge_t(200.0f,  40.0f, -150.0f, 200.0f, 100.0f, -150.0f)); // Line  6 (Pt  5 -  9)
	edges.push_back(edge_t(  0.0f, 100.0f, -100.0f, 200.0f, 100.0f, -100.0f)); // Line  7 (Pt  7 -  8)
	edges.push_back(edge_t(200.0f, 100.0f, -100.0f, 200.0f, 100.0f, -150.0f)); // Line  8 (Pt  8 -  9)
	edges.push_back(edge_t(200.0f, 100.0f, -150.0f,   0.0f, 100.0f, -150.0f)); // Line  9 (Pt  9 - 10)
	edges.push_back(edge_t(  0.0f,  40.0f, -150.0f,   0.0f,  40.0f, -400.0f)); // Line 10 (Pt  6 - 11)
	edges.push_back(edge_t(  0.0f,  40.0f, -400.0f, 450.0f,  40.0f, -400.0f)); // Line 11 (Pt 11 - 12)
	edges.push_back(edge_t(450.0f,  40.0f, -400.0f, 450.0f, 220.0f, -400.0f)); // Line 12 (Pt 12 - 13)
	edges.push_back(edge_t(450.0f, 220.0f, -400.0f, 150.0f, 220.0f, -400.0f)); // Line 13 (Pt 13 - 14)
	edges.push_back(edge_t(150.0f, 220.0f, -400.0f, 150.0f, 100.0f, -400.0f)); // Line 14 (Pt 14 - 15)
	edges.push_back(edge_t(150.0f, 100.0f, -400.0f,   0.0f, 100.0f, -400.0f)); // Line 15 (Pt 15 - 16)
	edges.push_back(edge_t(  0.0f, 100.0f, -400.0f,   0.0f, 100.0f, -100.0f)); // Line 16 (Pt 16 -  7)
	edges.push_back(edge_t(600.0f, 250.0f, -400.0f,   0.0f, 250.0f, -400.0f)); // Line 17 (Pt 17 - 18)
	edges.push_back(edge_t(  0.0f, 250.0f, -400.0f,   0.0f, 250.0f,   0.0f)); // Line 18 (Pt 18 - 19)

	/*
	// TODO: Hack to fix y-coord
	for (size_t i = 0; i < edges.size(); i++)
	{
		edges[i].modelPoint[0].setY(-edges[i].modelPoint[0].y());
		edges[i].modelPoint[1].setY(-edges[i].modelPoint[1].y());
	}
	*/

	// Add offset
	FVector4f offset(m_offset.x(), m_offset.y(), m_offset.z(), 0.0f);
	for (size_t i = 0, n = edges.size(); i < n; i++)
	{
		edges[i].modelPoint[0] += offset;
		edges[i].modelPoint[1] += offset;
	}
}

void FStudioModelVirtEco::onCreateFaces(faceVec_t& faces)
{
}

// ----------------------------------------------------------------------------------------------------