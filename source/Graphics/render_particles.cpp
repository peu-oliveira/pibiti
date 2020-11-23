#include "header.h"
#include "curvatureflow.h"
#include "render_particles.h"
#include "stb_image.h"

unsigned int cubemapTexture;
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
		/*glUniform1f( m_uLocStepsS, m_fSteps );*/
	}

	glColor3f(1, 1, 1);
	_drawPoints();

	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
}
void ParticleRenderer::display_CF(bool FB)
{
	int SCR_WIDTH = glutGet(GLUT_WINDOW_WIDTH), SCR_HEIGHT = glutGet(GLUT_WINDOW_HEIGHT);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glDepthMask(GL_TRUE);	glEnable(GL_DEPTH_TEST);

	int i = m_nProg;
	glUseProgram(m_program[i]);	//  pass vars
	glUniform1i(glGetUniformLocation(m_program[i], "depth"), 0);
	glUniform1i(glGetUniformLocation(m_program[i],"gPosition"),1);
	glUniform1i(glGetUniformLocation(m_program[i], "gNormal"), 2);
	glUniform1i(glGetUniformLocation(m_program[i], "gAlbedoSpec"), 3);

	if (FB == 1) {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, depth);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	}
	else {
		glActiveTexture(GL_TEXTURE0);
		glBindTexture(GL_TEXTURE_2D, Zvalue);
		glActiveTexture(GL_TEXTURE3);
		glBindTexture(GL_TEXTURE_2D, gAlbedoSpec);
	}
	glActiveTexture(GL_TEXTURE1);
	glBindTexture(GL_TEXTURE_2D, gPosition);
	glActiveTexture(GL_TEXTURE2);
	glBindTexture(GL_TEXTURE_2D, gNormal);

	glUniform1f( m_uLocPScale[i],  m_ParScale );
	glUniform1f( m_uLocPRadius[i], m_ParRadius );
	if (i==0)	{
		glUniform1f( m_uLocDiffuse, m_fDiffuse );
		glUniform1f( m_uLocAmbient, m_fAmbient );
		glUniform1f( m_uLocPower,   m_fPower ); 
		glUniform1f(scrH,SCR_HEIGHT);
		glUniform1f(scrW,SCR_WIDTH);
	}
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
	int SCR_WIDTH = glutGet(GLUT_WINDOW_WIDTH), SCR_HEIGHT = glutGet(GLUT_WINDOW_HEIGHT);
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
	glDrawBuffers(3, attachments);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);

	glGenFramebuffers(1, &depthFB);
	glBindFramebuffer(GL_FRAMEBUFFER, depthFB);
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

void ParticleRenderer::drawCubemap()
{
		glDepthFunc(GL_LEQUAL);  // change depth function so depth test passes when values are equal to depth buffer's content
	/*skyboxShader.use();
	view = glm::mat4(glm::mat3(camera.GetViewMatrix())); // remove translation from the view matrix
	skyboxShader.setMat4("view", view);
	skyboxShader.setMat4("projection", projection);
	// skybox cube
	glBindVertexArray(skyboxVAO);
	glActiveTexture(GL_TEXTURE0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, cubemapTexture);
	glDrawArrays(GL_TRIANGLES, 0, 36);
	glBindVertexArray(0);
	glDepthFunc(GL_LESS); // set depth function back to default*/
}


void ParticleRenderer::DepthBufUse()
{
	if (isTexC == 0) {
		createTexture();
		isTexC = 1;
	}
	glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	glEnable(GL_POINT_SPRITE_ARB);
	glTexEnvi(GL_POINT_SPRITE_ARB, GL_COORD_REPLACE_ARB, GL_TRUE);
	glEnable(GL_VERTEX_PROGRAM_POINT_SIZE_NV);
	glEnable(GL_DEPTH_TEST);

	glUseProgram(gbufferProg);	//  pass vars
	glUniform1f(gPscale, m_ParScale);
	glUniform1f(gPradius, m_ParRadius);
	glUniform1f(gm_uLocDiffuse, m_fDiffuse);
	glUniform1f(gm_uLocAmbient, m_fAmbient);
	glUniform1f(gm_uLocPower, m_fPower);

	glColor3f(1, 1, 1);
	_drawPoints();
	glUseProgram(0);
	glDisable(GL_POINT_SPRITE_ARB);
	glBindFramebuffer(GL_FRAMEBUFFER, 0);
	int nIter = 4;
	bool isFB = 0;
	for (int i = 0; i < nIter; i++) {
		isFB = !isFB;
		glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
		if(isFB==1) glBindFramebuffer(GL_FRAMEBUFFER, depthFB);
		else glBindFramebuffer(GL_FRAMEBUFFER, gBuffer);
		display_CF(isFB);
		glBindFramebuffer(GL_FRAMEBUFFER, 0);
	}
	glClear(GL_COLOR_BUFFER_BIT | GL_DEPTH_BUFFER_BIT);
	display_CF(0);
}

 /*unsigned int ParticleRenderer::loadCubemap(vector<std::string> faces)
{
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

	return textureID;
}*/

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

	vector<std::string> faces
	{
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/PositiveX.png"),
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/NegativeX.png"),
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/PositiveY.png"),
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/NegativeY.png"),
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/PositiveZ.png"),
		("C:/Users/Usuario/source/repos/Project1/Uploads/skybox/skybox/NegativeZ.png"),
	};
	//cubemapTexture = loadCubemap(faces);

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

GLuint ParticleRenderer::_compileProgramA(const char *vsource, const char *fsource)
{
	GLuint vertexShader;
	if (vsource) {
		vertexShader = glCreateShader(GL_VERTEX_SHADER);
		glShaderSource(vertexShader, 1, &vsource, 0);
		glCompileShader(vertexShader);
	}

	GLuint fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
	glShaderSource(fragmentShader, 1, &fsource, 0);
	glCompileShader(fragmentShader);

	GLuint program = glCreateProgram();

	if (vsource)	glAttachShader(program, vertexShader);
	glAttachShader(program, fragmentShader);

		glBindFragDataLocation(program, 0, "gPosition");
		glBindFragDataLocation(program, 1, "gNormal");
		glBindFragDataLocation(program, 2, "gAlbedoSpec");

	glLinkProgram(program);

	// check if program linked
	GLint success = 0;
	glGetProgramiv(program, GL_LINK_STATUS, &success);

	if (!success) {
		char temp[256];
		glGetProgramInfoLog(program, 256, 0, temp);
		printf("Failed to link program:\n%s\n", temp);
		glDeleteProgram(program);	program = 0;
	}
	return program;
}

void ParticleRenderer::_initGL()
{
	m_scaleProg = _compileProgram(NULL, scalePixelShader);
	gbufferProg = _compileProgramA(GvertexShader, GfragmentShader);
	
	for (int i=0; i<NumProg; i++)	{
		m_program[i] = _compileProgram(vertexShader, spherePixelShader[i]);

		//  vars loc
		m_uLocPScale[i]  = glGetUniformLocation(m_program[i], "pointScale");
		m_uLocPRadius[i] = glGetUniformLocation(m_program[i], "pointRadius");	}
	if (Pedro) {
		for (int i = 0; i < NumProg; i++) {
			m_program[i] = _compileProgram(vertexShader_Pedro, spherePixelShader_Pedro[i]);

			//  vars loc
			m_uLocPScale[i] = glGetUniformLocation(m_program[i], "pointScale");
			m_uLocPRadius[i] = glGetUniformLocation(m_program[i], "pointRadius");
		}
	}

	scrH = glGetUniformLocation(m_program[0], "SCR_HEIGHT");
	scrW = glGetUniformLocation(m_program[0], "SCR_WIDTH");
	m_uLocHueDiff = glGetUniformLocation(m_program[1], "fHueDiff");
	m_uLocDiffuse = glGetUniformLocation(m_program[0], "fDiffuse");
	m_uLocAmbient = glGetUniformLocation(m_program[0], "fAmbient");
	m_uLocPower   = glGetUniformLocation(m_program[0], "fPower");
	gm_uLocDiffuse = glGetUniformLocation(gbufferProg, "fDiffuse");
	gm_uLocAmbient = glGetUniformLocation(gbufferProg, "fAmbient");
	gm_uLocPower = glGetUniformLocation(gbufferProg, "fPower");
	
	m_uLocSteps	= glGetUniformLocation(m_program[1], "fSteps");
	m_uLocStepsS= glGetUniformLocation(m_scaleProg, "fSteps");

	gPradius = glGetUniformLocation(gbufferProg, "pointScale");
	gPscale = glGetUniformLocation(gbufferProg, "pointRadius");

	glClampColorARB(GL_CLAMP_VERTEX_COLOR_ARB, GL_FALSE);
	glClampColorARB(GL_CLAMP_FRAGMENT_COLOR_ARB, GL_FALSE);
}
