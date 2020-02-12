// ----------------------------------------------------------------------------------------------------
//  Title			FRenderTest.h
//  Description		Header file for FRenderTest.cpp
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#ifndef FRENDERTEST_H
#define FRENDERTEST_H

#include "FTrackMe.h"
#include "FlowGL.h"
#include "FMatrix4T.h"
#include "FStopWatch.h"

// ----------------------------------------------------------------------------------------------------
//  Class FRenderTest
// ----------------------------------------------------------------------------------------------------

class FRenderTest
{
	//  Constructors and destructor --------------------------------------------

public:
	/// Default Constructor.
	FRenderTest();
	/// Virtual destructor.
	virtual ~FRenderTest();

	//  Public commands --------------------------------------------------------

public:
	void init();
	void render();
	void cleanup();
	void resize(const QSize& size);

	//  Public queries ---------------------------------------------------------

	//  Overridables -----------------------------------------------------------

protected:

	//  Overrides --------------------------------------------------------------

protected:

	//  Internal functions -----------------------------------------------------

private:

	//  Internal data members --------------------------------------------------

private:
	FGLShader m_vertexShader;
	FGLShader m_fragmentShader;
	FGLProgram m_program;
	FGLBuffer m_vertexBuffer;
	FGLBuffer m_indexBuffer;
	FGLVertexArray m_vertexArray;
	FGLTexture2D m_texture;
	FGLTexture2D m_texture2;

	FGLMesh m_mesh;
	FMeshBox m_box;
	FMeshSphere m_sphere;

    struct transform_t
    {
        FMatrix4f matModelView;
        FMatrix4f matModelViewProjection;
    };

    struct light_t
    {
        FVector4f vecDirection[2];
        FVector4f vecColor[2];
    };

	FMatrix4f m_matProjection;
    transform_t m_transform;
    light_t m_light;

    FGLBuffer m_ubTransform;
    FGLBuffer m_ubLight;

	FStopWatch m_stopWatch;
};
	
// ----------------------------------------------------------------------------------------------------

#endif // FRENDERTEST_H