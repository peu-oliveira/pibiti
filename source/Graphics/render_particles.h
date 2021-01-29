#pragma once

#define NumProg 2

class ParticleRenderer
{
public:
	ParticleRenderer();  ~ParticleRenderer();
	bool Curv_Flow_Render = 0; //** Set the rendering method
	int RenderMethod = 0;
	void changeBool(); //** Change boolean
	void raisenIter(); //** Change number of iterations
	void lownIter();
	int ReturnNIter();
	int nIter = 0;
	const int KernelC = 7;
	float kernel[15]; //** KernelC*2+1;
	void display();
	void display_CF(bool FB);
	void display_BF(bool FB);
	void createTexture();
	void renderQuad();
	void createQuad();
	void drawCubemap();
	void CurvatureFlow_Use();
	void BilateralFilter_Use();
	void ScreenSpaceRender(bool FB);
	void SetFoam();
	void ScreenSpaceSet();
	void cubemap();
	void CreateFilter();
	//  set
	void setPositions(float *pos, int nPar) { m_pos = pos;	m_numParticles = nPar; }
	void setVertexBuffer(uint vbo, int nPar) { m_vbo = vbo;	m_numParticles = nPar; }
	void setColorBuffer(uint vbo) { m_colorVbo = vbo; }

	void setFOV(float fov) { m_fov = fov;  updPScale(); }
	void setWindowSize(int w, int h) { m_window_w = w;  m_window_h = h;  updPScale(); }
	void updPScale() { m_ParScale = m_window_h / tanf(m_fov*0.5f*PI / 180.0f); }

protected:  // methods

	void _initGL();
	void _drawPoints();
	GLuint _compileProgram(const char *vsource, const char *fsource), _compileProgramA(const char *vsource, const char *fsource);

protected:  // data
public:
	int m_nProg;

	float *m_pos;	GLuint m_program[NumProg], m_program1[NumProg], m_vbo, m_colorVbo, m_scaleProg, gbufferProg, SkyboxProg, BFProg,SPRenderProg,FoamProg;
	unsigned int skyboxVAO, quadVAO = 0;
	int m_numParticles, m_window_w, m_window_h;  float m_fov;

	float m_ParRadius, m_ParScale, m_fDiffuse, m_fAmbient, m_fPower, m_fSteps, m_fHueDiff;
	GLint m_uLocPRadius[NumProg], m_uLocPScale[NumProg], m_uLocDiffuse, gm_uLocDiffuse, m_uLocAmbient, gm_uLocAmbient, m_uLocPower, gm_uLocPower, m_uLocSteps, m_uLocStepsS, m_uLocHueDiff, gPscale, gPradius, scrW, scrH;
	GLint m_uLocPRadius1[NumProg], m_uLocPScale1[NumProg], m_uLocDiffuse1, gm_uLocDiffuse1, m_uLocAmbient1, gm_uLocAmbient1, m_uLocPower1, gm_uLocPower1, m_uLocSteps1, m_uLocStepsS1, m_uLocHueDiff1, scrW1, scrH1;
	GLint SigmaDomain, KernelCenter,KernelUni[15],camerax,cameray,cameraz, SPcamerax, SPcameray, SPcameraz,SPHEIGHT,SPWIDTH,Projection,ModelView,FHEIGHT,FWIDTH,FProjection;

	unsigned int gBuffer, depth, gPosition, gNormal, particleThickness, depthFB, Zvalue, ColorMap, depth2,FoamFB,FoamDepth,FoamTex;
	GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };
	GLenum Dattachment[2] = { GL_COLOR_ATTACHMENT1,GL_COLOR_ATTACHMENT2 };
	GLenum FoamAttachment[1] = { GL_COLOR_ATTACHMENT0 };
	
	unsigned int rboDepth;
	int SCR_WIDTH, SCR_HEIGHT;

	static char *vertexShader; //** Original shaders
	static char *spherePixelShader[NumProg]; //** Original shaders
	static char *scalePixelShader; //** Original shaders
};