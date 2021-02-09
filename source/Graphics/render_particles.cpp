#include "header.h"
#include "render_particles.h"
#define STB_IMAGE_IMPLEMENTATION
#include "stb_image.h"
#include <fstream>
#include <sstream> 
#include "../App/App.h"
#include "../CUDA/Params.cuh"

unsigned int cubemapTexture;
bool isTexCreated = 0;

ParticleRenderer::ParticleRenderer() : m_pos(0), m_numParticles(0), m_ParRadius(0.04f), m_ParScale(1.f),
									   m_fDiffuse(0.3f), m_fAmbient(0.7f), m_fPower(1.f), m_fSteps(0), m_fHueDiff(0.f),
									   m_vbo(0), m_colorVbo(0)
{
	m_nProg = 0 /*0*/;
	m_program[0] = 0;
	m_program1[0] = 0;
	gbufferProg = 0;
	_initGL();
}

ParticleRenderer::~ParticleRenderer() { m_pos = 0; }

void ParticleRenderer::changeBool() {
	if (RenderMethod<2)
		RenderMethod++;
	else RenderMethod = 0;
	nIter = 0;
}

void ParticleRenderer::raisenIter() {
	nIter++;
}

void ParticleRenderer::lownIter() {
	if(nIter>0) nIter--;
}

int ParticleRenderer::ReturnNIter() {
	return nIter;
}

const std::string parseFileToString(const char* fileName)
{
	std::fstream fileStream;
	fileStream.open(fileName, std::fstream::in);
	std::stringstream fileSource;
	fileSource << fileStream.rdbuf();
		return  fileSource.str();
}
//** Original rendering [drawPoints() and display()]
void ParticleRenderer::_drawPoints()
{
	if (!m_vbo)
	{
		glBegin(GL_POINTS);
		int a = 0;
		for (int i = 0; i < m_numParticles; ++i, a += 4)
			glVertex3fv(&m_pos[a]);
		glEnd();
	}
	else
	{
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		glVertexPointer(4, GL_FLOAT, 0, 0);
		glEnableClientState(GL_VERTEX_ARRAY);

		if (m_colorVbo)
		{
			glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_colorVbo);
			glColorPointer(4, GL_FLOAT, 0, 0);
			glEnableClientState(GL_COLOR_ARRAY);
		}

		glDrawArrays(GL_POINTS, 0, m_numParticles);
		glBindBufferARB(GL_ARRAY_BUFFER_ARB, 0);
		glDisableClientState(GL_VERTEX_ARRAY);
		glDisableClientState(GL_COLOR_ARRAY);
	}
}
void ParticleRenderer::display()
{
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);

	int i = m_nProg;
	glUseProgram(m_program[i]); //  pass vars
	glUniform1f(m_uLocPScale[i], m_ParScale);
	glUniform1f(m_uLocPRadius[i], m_ParRadius);
	if (i == 0)
	{
		glUniform1f(m_uLocDiffuse, m_fDiffuse);
		glUniform1f(m_uLocAmbient, m_fAmbient);
		glUniform1f(m_uLocPower, m_fPower);
	}
	else
	{
		glUniform1f(m_uLocHueDiff, m_fHueDiff);
		glUniform1f(m_uLocSteps, m_fSteps);
		/*glUniform1f( m_uLocStepsS, m_fSteps );*/
	}

	glColor3f(1, 1, 1);
	_drawPoints();

	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
}

//** Create textures
void ParticleRenderer::createTexture()
{
	cout << "tex created" << endl;
	SCR_WIDTH = glutGet(GLUT_WINDOW_WIDTH);
	SCR_HEIGHT = glutGet(GLUT_WINDOW_HEIGHT);
	glGenFramebuffers(1, &gBuffer);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	// depth buffer
	glGenTextures(1, &depth);
	glBindTexture(GL_TEXTURE_2D, depth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth, 0);
	//unsigned int attachments[1] = { GL_DEPTH_ATTACHMENT };
	// position color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gNormal, 0);
	// normal color buffer
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gPosition, 0);
	// color + specular color buffer
	glGenTextures(1, &particleThickness);
	glBindTexture(GL_TEXTURE_2D, particleThickness);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, particleThickness, 0);
	glDrawBuffers(3, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &depthFB);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFB);
	glGenTextures(1, &depth2);
	glBindTexture(GL_TEXTURE_2D, depth2);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, depth2, 0);
	glGenTextures(1, &Zvalue);
	glBindTexture(GL_TEXTURE_2D, Zvalue);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, Zvalue, 0);
	glGenTextures(1, &ColorMap);
	glBindTexture(GL_TEXTURE_2D, ColorMap);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, ColorMap, 0);
	glDrawBuffers(2, Dattachment);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &FoamFB);
	glBindFramebuffer(GL_FRAMEBUFFER, FoamFB);
	glGenTextures(1, &FoamDepth);
	glBindTexture(GL_TEXTURE_2D, FoamDepth);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_DEPTH_COMPONENT, SCR_WIDTH, SCR_HEIGHT, 0, GL_DEPTH_COMPONENT, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_DEPTH_ATTACHMENT, GL_TEXTURE_2D, FoamDepth, 0);
	glGenTextures(1, &FoamTex);
	glBindTexture(GL_TEXTURE_2D, FoamTex);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, FoamTex, 0);
	glDrawBuffers(1, FoamAttachment);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}
//** Create quad for screen-space rendering
void ParticleRenderer::createQuad()
{
	unsigned int quadVBO;
	if (quadVAO == 0)
	{
		float quadVertices[] = {
			// positions        // texture Coords
			-1.0f,
			1.0f / 1,
			0.0f,
			0.0f,
			1.0f,
			-1.0f,
			-1.0f / 1,
			0.0f,
			0.0f,
			0.0f,
			1.0f,
			1.0f / 1,
			0.0f,
			1.0f,
			1.0f,
			1.0f,
			-1.0f / 1,
			0.0f,
			1.0f,
			0.0f,
		};
		// setup plane VAO
		glGenVertexArrays(1, &quadVAO);
		glGenBuffers(1, &quadVBO);
		glBindVertexArray(quadVAO);
		glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
		glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
		glEnableVertexAttribArray(0);
		glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)0);
		glEnableVertexAttribArray(1);
		glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void *)(3 * sizeof(float)));
		glBindVertexArray(0);
	}
}
//** Render quad for screen-space
void ParticleRenderer::renderQuad()
{
	glBindVertexArray(quadVAO);
	glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
	glBindVertexArray(0);
}
//** Draw cubemap
void ParticleRenderer::drawCubemap()
{
	glDepthFunc(GL_LEQUAL); // change depth function so depth test passes when values are equal to depth buffer's content
	glUseProgram(SkyboxProg);
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glUseProgram(0);
	glDepthFunc(GL_LESS); // set depth function back to default*/
}
//** Steps for curvature flow rendering
void ParticleRenderer::ScreenSpaceSet()
{
	glGetFloatv(GL_PROJECTION_MATRIX, Pmatrix);
	glGetFloatv(GL_MODELVIEW_MATRIX, MVmatrix);
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	drawCubemap();
	glClear(GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glEnable(GL_DEPTH_TEST);
	glEnable(GL_BLEND); 
	glBlendFunc(GL_SRC_ALPHA, GL_ONE_MINUS_SRC_ALPHA);

	glUseProgram(gbufferProg); //  pass vars
	glUniform1f(gPscale, m_ParScale);
	glUniform1f(gPradius, m_ParRadius);
	glUniform1f(gm_uLocDiffuse, m_fDiffuse);
	glUniform1f(gm_uLocAmbient, m_fAmbient);
	glUniform1f(gm_uLocPower, m_fPower);

	//glColor3f(1, 1, 1);
	_drawPoints();
/*	glClear(GL_COLOR_BUFFER_BIT);
	glDisable(GL_DEPTH_TEST);
	glDepthMask(GL_FALSE);
	_drawPoints();*/
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
	glDisable(GL_BLEND);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	SetFoam();

	//glDepthMask(GL_FALSE);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glDepthMask(GL_TRUE);
	glEnable(GL_DEPTH_TEST);
	SCR_WIDTH = glutGet(GLUT_WINDOW_WIDTH);
	SCR_HEIGHT = glutGet(GLUT_WINDOW_HEIGHT);
	createQuad();
	if(RenderMethod==2) BilateralFilter_Use(); //**Still not working properly
	if(RenderMethod==1) CurvatureFlow_Use();
	drawCubemap();
}

void ParticleRenderer::SetFoam() {
	glBindFramebuffer(GL_FRAMEBUFFER, FoamFB);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(FoamProg); //  pass vars
	glUniform1f(FHEIGHT, SCR_HEIGHT);
	glUniform1f(FWIDTH, SCR_WIDTH);
	glUniform1f(FScale, m_ParScale);
	glUniform1f(FRadius, m_ParRadius);
	glUniformMatrix4fv(FProjection, 1, GL_FALSE, Pmatrix);
	glUniformMatrix4fv(FModelView, 1, GL_FALSE, MVmatrix);

	_drawPoints();
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticleRenderer::ScreenSpaceRender(bool FB) {
//	glEnable(GL_FRAMEBUFFER_SRGB);
	Scene &q = App::psys->scn;
	GLfloat camPosx, camPosy, camPosz;
	camPosx = App::psys->scn.camPos.x;
	camPosy = App::psys->scn.camPos.y;
	camPosz = App::psys->scn.camPos.z;
	glUseProgram(SPRenderProg);
	glUniform1f(SPcamerax, camPosx);
	glUniform1f(SPcameray, camPosy);
	glUniform1f(SPcameraz, camPosz);
	glUniformMatrix4fv(Projection, 1 ,GL_FALSE,Pmatrix);
	glUniformMatrix4fv(ModelView, 1, GL_FALSE, MVmatrix);
	glUniform1i(glGetUniformLocation(SPRenderProg, "depth"), 0);
	glUniform1i(glGetUniformLocation(SPRenderProg, "particleThickness"), 1);
	glUniform1i(glGetUniformLocation(SPRenderProg, "gPosition"), 2);
	glUniform1i(glGetUniformLocation(SPRenderProg, "FoamDepthTexture"), 3);
	glUniform1i(glGetUniformLocation(SPRenderProg, "skybox"), 4);
	glUniform1i(glGetUniformLocation(SPRenderProg, "FoamDepthStencilTexture"), 5);
	if (FB == 1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depth);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gPosition);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depth2);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gPosition);
	}
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, particleThickness);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, FoamDepth);
	glActiveTexture(GL_TEXTURE5);
	glBindTexture(GL_TEXTURE_2D, FoamTex);
	glUniform1f(SPHEIGHT, SCR_HEIGHT);
	glUniform1f(SPWIDTH, SCR_WIDTH);

	renderQuad();
//	glDisable(GL_FRAMEBUFFER_SRGB);
	glUseProgram(0);
}

void ParticleRenderer::BilateralFilter_Use() {
	bool FB = 1;

	float sigma = 0.01;
	kernel;
	KernelC;
	glUseProgram(BFProg);

	glUniform1i(glGetUniformLocation(BFProg, "DepthTexture"), 0);
	glUniform1i(glGetUniformLocation(BFProg, "particleThickness"), 1);
	glUniform1i(glGetUniformLocation(BFProg, "gPosition"), 2);
	glUniform1f(scrH, SCR_HEIGHT);
	glUniform1f(scrW, SCR_WIDTH);
	glUniform1f(SigmaDomain, sigma);
	//glUniform1f(SigmaDomain, sigma);
	glUniform1i(KernelCenter, KernelC);

	for (int i = 0; i <= KernelC * 2 + 1; i++) {
		glUniform1f(KernelUni[i], kernel[i]);
	}

	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, particleThickness);

	for (int i = 0; i < nIter; i++) {
		if (FB == 1)
			glBindFramebuffer(GL_FRAMEBUFFER, depthFB);
		else
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		display_BF(FB);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		FB = !FB;
	}
	glClear( GL_DEPTH_BUFFER_BIT);
	glUseProgram(0);

	ScreenSpaceRender(FB);
}

void ParticleRenderer::display_BF(bool FB) {
	glActiveTexture(GL_TEXTURE0);
	if (FB == 1) glBindTexture(GL_TEXTURE_2D, depth);
	else glBindTexture(GL_TEXTURE_2D, depth2);
	renderQuad();
}

void ParticleRenderer::CurvatureFlow_Use()
{
bool FBnum = 1;

	for (int i = 0; i < nIter; i++)
	{
		if (FBnum == 1)
			glBindFramebuffer(GL_FRAMEBUFFER, depthFB);
		else
			glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		glClear(GL_DEPTH_BUFFER_BIT);
		display_CF(FBnum);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
		FBnum = !FBnum;
}
glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
//glDepthMask(GL_TRUE);
int i = m_nProg;
glUseProgram(m_program1[i]);

ScreenSpaceRender(FBnum);
}

//** Render using curvature flow
void ParticleRenderer::display_CF(bool FB)
{
	int i = m_nProg;
	glUseProgram(m_program1[i]); //  pass vars
	glUniform1i(glGetUniformLocation(m_program1[i], "depth"), 0);
	glUniform1i(glGetUniformLocation(m_program1[i], "gNormal"), 1);
	glUniform1i(glGetUniformLocation(m_program1[i], "gPosition"), 2);
	glUniform1i(glGetUniformLocation(m_program1[i], "particleThickness"), 3);
	glUniform1i(glGetUniformLocation(m_program1[i], "skybox"), 4);

	if (FB == 1)
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depth);
		glActiveTexture(GL_TEXTURE2);
		glBindTexture(GL_TEXTURE_2D, gPosition);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depth2);
		glActiveTexture(GL_TEXTURE2);
	    glBindTexture(GL_TEXTURE_2D, gPosition);
	}
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glActiveTexture(GL_TEXTURE3);
	glBindTexture(GL_TEXTURE_2D, particleThickness);
	glActiveTexture(GL_TEXTURE4);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);

	glUniform1f(m_uLocPScale1[i], m_ParScale);
	glUniform1f(m_uLocPRadius1[i], m_ParRadius);
	if (i == 0)
	{
		glUniform1f(m_uLocDiffuse1, m_fDiffuse);
		glUniform1f(m_uLocAmbient1, m_fAmbient);
		glUniform1f(m_uLocPower1, m_fPower);
		glUniform1f(scrH1, SCR_HEIGHT);
		glUniform1f(scrW1, SCR_WIDTH);
	}
	else
	{
		glUniform1f(m_uLocHueDiff1, m_fHueDiff);
		glUniform1f(m_uLocSteps1, m_fSteps);
		/*glUniform1f( m_uLocStepsS, m_fSteps );*/
	}

	//glColor3f(1, 1, 1);
	renderQuad();

	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
}
//** Create cubemap
void ParticleRenderer::cubemap()
{
	float skyboxVertices[] = {
		// positions
		-1.0f, 1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, -1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, -1.0f, 1.0f,
		-1.0f, -1.0f, 1.0f,

		-1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, -1.0f,
		1.0f, 1.0f, 1.0f,
		1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, 1.0f,
		-1.0f, 1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f, 1.0f,
		1.0f, -1.0f, 1.0f};

	// skybox VAO
	unsigned int skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void *)0);

	vector<std::string> faces{
		("assets/textures/px.png"),
		("assets/textures/nx.png"),
		("assets/textures/py.png"),
		("assets/textures/ny.png"),
		("assets/textures/pz.png"),
		("assets/textures/nz.png"),
	};
	unsigned int textureID;
	glGenTextures(1, &textureID);
	glBindTexture(GL_TEXTURE_CUBE_MAP, textureID);

	int width, height, nrComponents;
	for (unsigned int i = 0; i < faces.size(); i++)
	{
		unsigned char *data = stbi_load(faces[i].c_str(), &width, &height, &nrComponents, 0);
		if (data)
		{
			glTexImage2D(GL_TEXTURE_CUBE_MAP_POSITIVE_X + i, 0, GL_RGB, width, height, 0, GL_RGB, GL_UNSIGNED_BYTE, data);
			stbi_image_free(data);
		}
		else
		{
			std::cout << "Cubemap texture failed to load at path: " << faces[i] << std::endl;
			stbi_image_free(data);
		}
	}
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_S, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_T, GL_CLAMP_TO_EDGE);
	glTexParameteri(GL_TEXTURE_CUBE_MAP, GL_TEXTURE_WRAP_R, GL_CLAMP_TO_EDGE);

	cubemapTexture = textureID;
	glBindVertexArray(0);
}

GLuint ParticleRenderer::_compileProgram(const char *vsource, const char *fsource)
{
	GLuint vertexShader;
	if (vsource)
	{
		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vsource, 0);
		glCompileShader(vertexShader);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fsource, 0);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();

	if (vsource)
		glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	// check if program linked
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success)
	{
		char temp[1024];
		glGetProgramInfoLog(program, 1024, 0, temp);
		printf("Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);
		program = 0;
	}
	return program;
}

GLuint ParticleRenderer::_compileProgramA(const char *vsource, const char *fsource)
{
	GLuint vertexShader;
	if (vsource)
	{
		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vsource, 0);
		glCompileShader(vertexShader);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fsource, 0);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();

	if (vsource)
		glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glBindFragDataLocation(program, 0, "gNormal");
	glBindFragDataLocation(program, 1, "gPosition");
	glBindFragDataLocation(program, 2, "particleThickness");

	glLinkProgram(program);

	// check if program linked
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success)
	{
		char temp[1024];
		glGetProgramInfoLog(program, 1024, 0, temp);
		printf("Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);
		program = 0;
	}
	return program;
}

void ParticleRenderer::CreateFilter() {
	for (int i = -KernelC; i <= KernelC; i++) {
		kernel[KernelC + i] = exp(-i*i*0.1);
	}
}

void ParticleRenderer::_initGL()
{
	cubemap(); //** Init cubemap
	createTexture(); //** Init texture
	CreateFilter(); //** Init filter for BF


	//** Compile shaders
	m_scaleProg = _compileProgram(NULL, scalePixelShader);
	gbufferProg = _compileProgramA(parseFileToString("source/Graphics/shaders/Gbuffer.vs").c_str(), parseFileToString("source/Graphics/shaders/Gbuffer.fs").c_str());
	SkyboxProg = _compileProgram(parseFileToString("source/Graphics/shaders/Cubemap.vs").c_str(), parseFileToString("source/Graphics/shaders/Cubemap.fs").c_str());
	BFProg = _compileProgram(parseFileToString("source/Graphics/shaders/ScreenSpace.vs").c_str(), parseFileToString("source/Graphics/shaders/BilateralFilter.fs").c_str());
	SPRenderProg = _compileProgram(parseFileToString("source/Graphics/shaders/ScreenSpace.vs").c_str(), parseFileToString("source/Graphics/shaders/ScreenSpaceRender.fs").c_str());
	FoamProg = _compileProgramA(parseFileToString("source/Graphics/shaders/ParticleRender.vs").c_str(), parseFileToString("source/Graphics/shaders/Foam.fs").c_str());
	//cout << parseFileToString("source/Graphics/shaders/CurvatureFlow.fs").c_str();

	//** Original rendering
	for (int i = 0; i < NumProg; i++)
	{
		m_program[i] = _compileProgram(vertexShader, spherePixelShader[i]);

		//  vars loc
		m_uLocPScale[i] = glGetUniformLocation(m_program[i], "pointScale");
		m_uLocPRadius[i] = glGetUniformLocation(m_program[i], "pointRadius");
	}

	//** CF rendering
	for (int i = 0; i < NumProg; i++)
	{
		m_program1[i] = _compileProgram(parseFileToString("source/Graphics/shaders/ScreenSpace.vs").c_str(), parseFileToString("source/Graphics/shaders/CurvatureFlow.fs").c_str());

		//  vars loc
		m_uLocPScale1[i] = glGetUniformLocation(m_program1[i], "pointScale");
		m_uLocPRadius1[i] = glGetUniformLocation(m_program1[i], "pointRadius");
	}
	
//** Uniforms for original rendering
	m_uLocHueDiff = glGetUniformLocation(m_program[1], "fHueDiff");
	m_uLocDiffuse = glGetUniformLocation(m_program[0], "fDiffuse");
	m_uLocAmbient = glGetUniformLocation(m_program[0], "fAmbient");
	m_uLocPower = glGetUniformLocation(m_program[0], "fPower");
	m_uLocSteps = glGetUniformLocation(m_program[1], "fSteps");

	m_uLocStepsS = glGetUniformLocation(m_scaleProg, "fSteps");

//** Uniforms for CF rendering
	scrH1 = glGetUniformLocation(m_program1[0], "SCR_HEIGHT");
	scrW1 = glGetUniformLocation(m_program1[0], "SCR_WIDTH");
	camerax = glGetUniformLocation(m_program1[0], "camPosx");
	cameray = glGetUniformLocation(m_program1[0], "camPosy");
	cameraz = glGetUniformLocation(m_program1[0], "camPosz");
	m_uLocHueDiff1 = glGetUniformLocation(m_program1[1], "fHueDiff");
	m_uLocDiffuse1 = glGetUniformLocation(m_program1[0], "fDiffuse");
	m_uLocAmbient1 = glGetUniformLocation(m_program1[0], "fAmbient");
	m_uLocPower1 = glGetUniformLocation(m_program1[0], "fPower");
	m_uLocSteps1 = glGetUniformLocation(m_program1[1], "fSteps");

	gm_uLocDiffuse = glGetUniformLocation(gbufferProg, "fDiffuse");
	gm_uLocAmbient = glGetUniformLocation(gbufferProg, "fAmbient");
	gm_uLocPower = glGetUniformLocation(gbufferProg, "fPower");
	gPradius = glGetUniformLocation(gbufferProg, "pointScale");
	gPscale = glGetUniformLocation(gbufferProg, "pointRadius");

//** Uniforms for BF rendering
	scrH = glGetUniformLocation(BFProg, "SCR_HEIGHT");
	scrW = glGetUniformLocation(BFProg, "SCR_WIDTH");
	SigmaDomain = glGetUniformLocation(BFProg, "DomainSigma");
	KernelCenter = glGetUniformLocation(BFProg, "KernelCenter");
	for (int i = 0; i <= KernelC * 2 + 1; i++) {
		std::string name = "Kernel[";
		name = name + to_string(i) + "]";
		KernelUni[i] = glGetUniformLocation(BFProg, name.c_str());
	}

//** Uniforms for screen space render
	Projection = glGetUniformLocation(SPRenderProg, "Projection");
	ModelView = glGetUniformLocation(SPRenderProg, "ModelView");
	SPHEIGHT = glGetUniformLocation(SPRenderProg, "SCR_HEIGHT");
	SPWIDTH = glGetUniformLocation(SPRenderProg, "SCR_WIDTH");
	SPcamerax = glGetUniformLocation(SPRenderProg, "camPosx");
	SPcameray = glGetUniformLocation(SPRenderProg, "camPosy");
	SPcameraz = glGetUniformLocation(SPRenderProg, "camPosz");

//** Uniforms for Foam shader
	FProjection = glGetUniformLocation(FoamProg, "Projection");
	FModelView = glGetUniformLocation(FoamProg, "ModelView");
	FHEIGHT = glGetUniformLocation(FoamProg, "SCR_HEIGHT");
	FWIDTH = glGetUniformLocation(FoamProg, "SCR_WIDTH");
	FRadius = glGetUniformLocation(FoamProg, "pointScale");
	FScale = glGetUniformLocation(FoamProg, "pointRadius");
	//GHEIGHT = glGetUniformLocation(FoamProg, "SCR_HEIGHT");

	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
}