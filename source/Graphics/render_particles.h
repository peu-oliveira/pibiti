#pragma once

#define NumProg 2


class ParticleRenderer
{
public:
	ParticleRenderer();  ~ParticleRenderer();

	void display();
	void createTexture();
	void renderQuad();
	void DepthBufUse();
	void cubemap();
	//  set
	void setPositions( float *pos, int nPar)	{	m_pos = pos;	m_numParticles = nPar;	}
	void setVertexBuffer(uint vbo, int nPar)	{	m_vbo = vbo;	m_numParticles = nPar;	}
	void setColorBuffer(uint vbo)				{	m_colorVbo = vbo;	}

	void setFOV(float fov)			{	m_fov = fov;  updPScale();	}
	void setWindowSize(int w, int h){	m_window_w = w;  m_window_h = h;  updPScale();	}
	void updPScale()	{	m_ParScale = m_window_h / tanf(m_fov*0.5f*PI/180.0f);	}

protected:  // methods

	void _initGL();
	void _drawPoints();
	GLuint _compileProgram(const char *vsource, const char *fsource, int atacch);

protected:  // data
public:
	int m_nProg;

	float *m_pos;	GLuint m_program[NumProg], m_vbo, m_colorVbo, m_scaleProg, gbufferProg;
	int m_numParticles,  m_window_w, m_window_h;  float m_fov;

	float m_ParRadius, m_ParScale,  m_fDiffuse, m_fAmbient, m_fPower,  m_fSteps, m_fHueDiff;
	GLint m_uLocPRadius[NumProg],m_uLocPScale[NumProg], m_uLocDiffuse, gm_uLocDiffuse, m_uLocAmbient,gm_uLocAmbient,m_uLocPower, gm_uLocPower, m_uLocSteps,m_uLocStepsS, m_uLocHueDiff,gPscale,gPradius;
	unsigned int gBuffer,depth, gPosition, gNormal, gAlbedoSpec;;
	GLenum attachments[3] = { GL_COLOR_ATTACHMENT0, GL_COLOR_ATTACHMENT1, GL_COLOR_ATTACHMENT2 };

	static char *vertexShader;
	static char *spherePixelShader[NumProg];
	static char *scalePixelShader;
	static char *GvertexShader;
	static char *GfragmentShader;
};

