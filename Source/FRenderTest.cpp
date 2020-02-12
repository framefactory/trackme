// ----------------------------------------------------------------------------------------------------
//  Title			FRenderTest.cpp
//  Description		Implementation of class FRenderTest
// ----------------------------------------------------------------------------------------------------
//  $Author: ralphw $
//  $Revision: 5 $
//  $Date: 2011-08-11 10:45:05 +0200 (Do, 11 Aug 2011) $
// ----------------------------------------------------------------------------------------------------

#include "FTrackMeStable.h"

#include "FRenderTest.h"
#include "FMemoryTracer.h"

// ----------------------------------------------------------------------------------------------------
//  Class FRenderTest
// ----------------------------------------------------------------------------------------------------

// Constructors and destructor ------------------------------------------------------------------------

FRenderTest::FRenderTest()
{
	init();
}

FRenderTest::~FRenderTest()
{
	cleanup();
}

// Public commands ------------------------------------------------------------------------------------

void FRenderTest::init()
{
	/*
	FGLContextSettings settings;
	settings.setVersion(3, 3);
	F_VERIFY(m_context.create(settings, m_pWidget));
	m_context.makeCurrent();
	*/
	glViewport(0, 0, 768, 576);

	glPixelStorei(GL_UNPACK_ALIGNMENT, 1);
	glDisable(GL_CULL_FACE);
	glEnable(GL_DEPTH_TEST);
	glDisable(GL_BLEND);

	glClearColor(0.0f, 0.0f, 0.5f, 1.0f);

	m_texture2.createAllocate(FGLPixelFormat::R8G8B8_UNorm, QSize(640, 480), 3);

	F_VERIFY(m_texture.createLoadFromFile("test.tif"));
	m_texture.generateMipmaps();
	m_texture.setFilter(FGLFilterType::Linear, FGLFilterType::Linear, FGLFilterType::Linear);
	F_TRACE(QString("Max Anisotropy: %1").arg(FGLTexture2D::maxAnisotropy()));

	QString vertexShaderFile("basicTransform.vsh");
	QString fragmentShaderFile("phongShader.psh");

	m_vertexShader.createCompileFromFile(vertexShaderFile, FGLShaderType::VertexShader);
	fInfo("OpenGL", QString("Vertex Shader (%1):\n%2")
		.arg(vertexShaderFile).arg(m_vertexShader.infoLog()));

	m_fragmentShader.createCompileFromFile(fragmentShaderFile, FGLShaderType::FragmentShader);
	fInfo("OpenGL", QString("Fragment Shader (%1):\n%2")
		.arg(fragmentShaderFile).arg(m_fragmentShader.infoLog()));

	m_program.create();
	m_program.attachShader(m_vertexShader);
	m_program.attachShader(m_fragmentShader);
	m_program.bindAttribLocation(0, "vecPosition");
	m_program.bindAttribLocation(1, "vecNormal");
	m_program.bindAttribLocation(2, "vecTexCoord");
	m_program.link();
	fInfo("OpenGL", QString("Linker Status:\n%0").arg(m_program.infoLog()));

	float vertices[] = { -1.0f, -1.0f, 0.0f, 1.0f, 0.0f, 1.0f,
						  1.0f, -1.0f, 0.0f, 1.0f, 1.0f, 1.0f,
						 -1.0f,  1.0f, 0.0f, 1.0f, 0.0f, 0.0f,
						  1.0f,  1.0f, 0.0f, 1.0f, 1.0f, 0.0f };

	m_vertexBuffer.create();
	m_vertexBuffer.initialize(vertices, 4 * 6 * sizeof(float), FGLUsage::StaticDraw);
	
	m_vertexArray.create();
	m_vertexArray.bindVertexBuffer(m_vertexBuffer);
	m_vertexArray.setVertexAttribute(
		0, 4, FGLDataType::Float, false, 6 * sizeof(float), 0);
	m_vertexArray.setVertexAttribute(
		1, 2, FGLDataType::Float, false, 6 * sizeof(float), (void*)(4 * sizeof(float)));

	m_box.setSize(5.0f, 5.0f, 5.0f);
	m_box.setPivot(2.5f, 2.5f, 2.5f);
	m_box.createMesh();

	m_sphere.setRadius(5.0f);
	m_sphere.setTesselation(48, 48);
	m_sphere.createMesh();

	m_mesh.create(m_sphere);
	//m_mesh.create(m_box);

    m_light.vecColor[0].set(1.0f, 1.0f, 0.0f, 1.0f);
    m_light.vecColor[1].set(0.0f, 0.5f, 1.0f, 1.0f);
    m_light.vecDirection[0].set(1.0f, 0.0f, 0.0f, 0.0f);
    m_light.vecDirection[1].set(-1.0f, 0.0f, 0.0f, 0.0f);
	m_light.vecDirection[0].normalize();
	m_light.vecDirection[1].normalize();

	FGLMath::makeProjectionPerspectiveLH(m_matProjection,
		1.5f, false, 1.0f, 0.1f, 1000.0f);

	m_transform.matModelView.makeIdentity();
	m_transform.matModelViewProjection = m_matProjection;
	m_transform.matModelViewProjection.transpose();

	m_ubLight.createInitialize(&m_light, sizeof(light_t), FGLUsage::DynamicDraw);
	m_ubTransform.createInitialize(&m_transform, sizeof(transform_t), FGLUsage::DynamicDraw);
	
	m_ubLight.bindUniform(1);
	m_ubTransform.bindUniform(0);

	m_program.bindUniformBlock("Light", 1);
	m_program.bindUniformBlock("Transform", 0);

	m_stopWatch.start();
}

void FRenderTest::render()
{
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);

	m_program.bind();
	m_texture.bind();
	
	double t = m_stopWatch.time();
	float angle = (float)t * 0.5f;

	m_transform.matModelView.makeRotationY(angle);
	FMatrix4f matTranslation;
	matTranslation.makeTranslation(0.0f, 0.0f, 8.0f);
	m_transform.matModelView = matTranslation * m_transform.matModelView;
	m_transform.matModelViewProjection = m_matProjection * m_transform.matModelView;

	m_transform.matModelView.transpose();
	m_transform.matModelViewProjection.transpose();

	m_ubTransform.write(&m_transform, sizeof(transform_t));
	
	//m_vertexArray.draw(FGLPrimitiveType::TriangleStrip, 0, 4);

	m_mesh.draw();
}

void FRenderTest::cleanup()
{
}

void FRenderTest::resize(const QSize& size)
{
	glViewport(0, 0, size.width(), size.height());
	float aspect = (float)size.width() / (float)size.height();
	FGLMath::makeProjectionPerspectiveLH(m_transform.matModelViewProjection,
		1.5f, false, aspect, 0.1f, 1000.0f);
}

// Public queries -------------------------------------------------------------------------------------

// Overrides ------------------------------------------------------------------------------------------

// Internal functions ---------------------------------------------------------------------------------

// ----------------------------------------------------------------------------------------------------