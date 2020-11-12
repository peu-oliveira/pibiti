#include "header.h"
#include "curvatureflow.h"
#include "render_particles.h"

bool isTexC=0;

ParticleRenderer::ParticleRenderer() :
	m_pos(0), m_numParticles(0), m_ParRadius(0.04f), m_ParScale(1.f),
	m_fDiffuse(0.3f), m_fAmbient(0.7f), m_fPower(1.f), m_fSteps(0), m_fHueDiff(0.f),
	m_vbo(0), m_colorVbo(0)
{
	m_nProg = 0/*0*/;  m_program[0] = 0; gbufferProg = 0;  _initGL();
}

ParticleRenderer::~ParticleRenderer()	{	m_pos = 0;	}



void ParticleRenderer::_drawPoints()
{
	if (!m_vbo)
	{	glBegin(GL_POINTS);  int a = 0;
		for (int i = 0; i < m_numParticles; ++i, a+=4)	glVertex3fv(&m_pos[a]);
		glEnd();	}
	else
	{	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_vbo);
		glVertexPointer(4, GL_FLOAT, 0, 0);
		glEnableClientState(GL_VERTEX_ARRAY);				

		if (m_colorVbo)
		{	glBindBufferARB(GL_ARRAY_BUFFER_ARB, m_colorVbo);
			glColorPointer(4, GL_FLOAT, 0, 0);
			glEnableClientState(GL_COLOR_ARRAY);	}

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
	glDepthMask(GL_TRUE);	glEnable(GL_DEPTH_TEST);

	int i = m_nProg;
	glUseProgram(m_program[i]);	//  pass vars
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_2D, depth);
	glUniform1f( m_uLocPScale[i],  m_ParScale );
	glUniform1f( m_uLocPRadius[i], m_ParRadius );
	if (i==0)	{
		glUniform1f( m_uLocDiffuse, m_fDiffuse );
		glUniform1f( m_uLocAmbient, m_fAmbient );
		glUniform1f( m_uLocPower,   m_fPower );  }
	else  {
		glUniform1f( m_uLocHueDiff, m_fHueDiff );
		glUniform1f( m_uLocSteps, m_fSteps );
		/*glUniform1f( m_uLocStepsS, m_fSteps );*/  }
	
	glColor3f(1, 1, 1);
	//_drawPoints();
	renderQuad();

	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
}

void ParticleRenderer::createTexture()
{
	cout << "tex created" << endl;
	int SCR_WIDTH = 800, SCR_HEIGHT = 600;
	gBuffer;
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
	glGenTextures(1, &gPosition);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT0, GL_TEXTURE_2D, gPosition, 0);
	// normal color buffer
	glGenTextures(1, &gNormal);
	glBindTexture(GL_TEXTURE_2D, gNormal);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA16F, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_FLOAT, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT1, GL_TEXTURE_2D, gNormal, 0);
	// color + specular color buffer
	glGenTextures(1, &gAlbedoSpec);
	glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA, SCR_WIDTH, SCR_HEIGHT, 0, GL_RGBA, GL_UNSIGNED_BYTE, NULL);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_NEAREST);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_NEAREST);
	glFramebufferTexture2D(GL_FRAMEBUFFER, GL_COLOR_ATTACHMENT2, GL_TEXTURE_2D, gAlbedoSpec, 0);
	glDrawBuffers(4, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
}

void ParticleRenderer::renderQuad()
{
	unsigned int quadVAO = 0;
	unsigned int quadVBO;
		if (quadVAO == 0)
		{
			float quadVertices[] = {
				// positions        // texture Coords
				-1.0f,  1.0f / 1, 0.0f, 0.0f, 1.0f,
				-1.0f, -1.0f / 1, 0.0f, 0.0f, 0.0f,
				1.0f,  1.0f / 1, 0.0f, 1.0f, 1.0f,
				1.0f, -1.0f / 1, 0.0f, 1.0f, 0.0f,
			};
			// setup plane VAO
			glGenVertexArrays(1, &quadVAO);
			glGenBuffers(1, &quadVBO);
			glBindVertexArray(quadVAO);
			glBindBuffer(GL_ARRAY_BUFFER, quadVBO);
			glBufferData(GL_ARRAY_BUFFER, sizeof(quadVertices), &quadVertices, GL_STATIC_DRAW);
			glEnableVertexAttribArray(0);
			glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)0);
			glEnableVertexAttribArray(1);
			glVertexAttribPointer(1, 2, GL_FLOAT, GL_FALSE, 5 * sizeof(float), (void*)(3 * sizeof(float)));
		}
		glBindVertexArray(quadVAO);
		glDrawArrays(GL_TRIANGLE_STRIP, 0, 4);
		glBindVertexArray(0);
}

void ParticleRenderer::DepthBufUse()
{
	if (isTexC == 0) {
		createTexture();
		isTexC = 1;
	}
	glBindFragDataLocation(gbufferProg, 0, "depth");
	glBindFragDataLocation(gbufferProg, 1, "gNormal");
	glBindFragDataLocation(gbufferProg, 2, "gPosition");
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(gbufferProg);	//  pass vars
	for (int i = 0; i < 5; i++) {

		//glActiveTexture(GL_TEXTURE0);
		//glBindTexture(GL_TEXTURE_2D, depth);
		m_ParRadius += i / 5;
		m_ParScale += i / 5;
		glUniform1f(gPscale, m_ParScale);
		glUniform1f(gPradius, m_ParRadius);
		glColor3f(1, 1, 1);
		_drawPoints();
	}
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	display();

	/*glMatrixMode(GL_PROJECTION);
	glPushMatrix();

	glLoadIdentity();
	gluOrtho2D(0, 1, 0, 1);
	math_init();
	glFlush();
	glReadPixels(0, 0, width, height, GL_DEPTH_COMPONENT, GL_FLOAT, depth_buffer);
	float fCFFactor = 0.01;
	unsigned nIterCount = 20;
	curvature_flow(depth_buffer, fCFFactor, width, height, nIterCount);

	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glDepthMask(GL_TRUE);	glEnable(GL_DEPTH_TEST);

	int i = m_nProg;
	glUseProgram(m_program[i]);	//  pass vars
	glUniform1f(m_uLocPScale[i], m_ParScale);
	glUniform1f(m_uLocPRadius[i], m_ParRadius);
	if (i == 0) {
		glUniform1f(m_uLocDiffuse, m_fDiffuse);
		glUniform1f(m_uLocAmbient, m_fAmbient);
		glUniform1f(m_uLocPower, m_fPower);
	}
	else {
		glUniform1f(m_uLocHueDiff, m_fHueDiff);
		glUniform1f(m_uLocSteps, m_fSteps);
		/*glUniform1f( m_uLocStepsS, m_fSteps );
	}

	glColor3f(1, 1, 1);
//	glBegin(GL_POINTS);  int a = 0;
	//	for (int i = 0; i < m_numParticles; ++i, a+=4)	glVertex3fv(&m_pos[a]);
		//lEnd();
/*	glBegin(GL_POINTS);
	for (int j = 0; j < height; j++)
		for (int i = 0; i < width; i++)
		{
			float fd = *(depth_buffer + (i + j * width));
			if (fd > 0)
			{
				glVertex2f(i * 1.f / width, j * 1.f / height);
			}
		}
	glEnd();
//	glPopMatrix();
	math_destroy();
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);*/
}

void ParticleRenderer::cubemap()
{
	float skyboxVertices[] = {
		// positions          
		-1.0f,  1.0f, -1.0f,
		-1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f, -1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,

		-1.0f, -1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f, -1.0f,  1.0f,
		-1.0f, -1.0f,  1.0f,

		-1.0f,  1.0f, -1.0f,
		1.0f,  1.0f, -1.0f,
		1.0f,  1.0f,  1.0f,
		1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f,  1.0f,
		-1.0f,  1.0f, -1.0f,

		-1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f, -1.0f,
		1.0f, -1.0f, -1.0f,
		-1.0f, -1.0f,  1.0f,
		1.0f, -1.0f,  1.0f
	};

	// skybox VAO
	unsigned int skyboxVAO, skyboxVBO;
	glGenVertexArrays(1, &skyboxVAO);
	glGenBuffers(1, &skyboxVBO);
	glBindVertexArray(skyboxVAO);
	glBindBuffer(GL_ARRAY_BUFFER, skyboxVBO);
	glBufferData(GL_ARRAY_BUFFER, sizeof(skyboxVertices), &skyboxVertices, GL_STATIC_DRAW);
	glEnableVertexAttribArray(0);
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (void*)0);



}


GLuint ParticleRenderer::_compileProgram(const char *vsource, const char *fsource)
{
	GLuint vertexShader;
	if (vsource)	{
	vertexShader = glCreateShader(GL_VERTEX_SHADER);
	glShaderSource(vertexShader, 1, &vsource, 0);
	glCompileShader(vertexShader);	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fsource, 0);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();

	if (vsource)	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

	glLinkProgram(program);

	// check if program linked
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {		char temp[256];
		glGetProgramInfoLog(program, 256, 0, temp);
		printf("Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);	program = 0;  }
	return program;
}


void ParticleRenderer::_initGL()
{
	m_scaleProg = _compileProgram(NULL, scalePixelShader);
	gbufferProg = _compileProgram(GvertexShader, GfragmentShader);
	
	for (int i=0; i<NumProg; i++)	{
		m_program[i] = _compileProgram(vertexShader, spherePixelShader[i]);
	//	m_program[i] = _compileProgram(GvertexShader, GfragmentShader);

		//  vars loc
		m_uLocPScale[i]  = glGetUniformLocation(m_program[i], "pointScale");
		m_uLocPRadius[i] = glGetUniformLocation(m_program[i], "pointRadius");	}

	m_uLocHueDiff = glGetUniformLocation(m_program[1], "fHueDiff");
	m_uLocDiffuse = glGetUniformLocation(m_program[0], "fDiffuse");
	m_uLocAmbient = glGetUniformLocation(m_program[0], "fAmbient");
	m_uLocPower   = glGetUniformLocation(m_program[0], "fPower");
	
	m_uLocSteps	= glGetUniformLocation(m_program[1], "fSteps");
	m_uLocStepsS= glGetUniformLocation(m_scaleProg, "fSteps");

	gPradius = glGetUniformLocation(gbufferProg, "pointScale");
	gPscale = glGetUniformLocation(gbufferProg, "pointRadius");
	isG = glGetUniformLocation(m_program[0], "isG");

	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
}
