#include "header.h"

#include "render_particles.h"


#define STRING(A) #A
#define STRING2(A,B) #A#B


///  Vertex shader

char *ParticleRenderer::vertexShader =
STRING(
#version 330 core \n
layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;
//uniform float  pointRadius;  // point size in world space
//uniform float  pointScale;   // scale to calculate size in pixels
out vec2 TexCoords;

void main()
{

	// calculate window-space point size
	/*vec3 posEye = vec3(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
	float dist = length(posEye);
	gl_PointSize = pointRadius * (pointScale / dist);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_FrontColor = gl_Color;*/
	TexCoords = aTexCoords;
	gl_Position = vec4(aPos, 1.0);
}
);

char *ParticleRenderer::GvertexShader =
STRING(

uniform float  pointRadius;  // point size in world space
uniform float  pointScale;   // scale to calculate size in pixels


void main()
{
	// calculate window-space point size
	vec3 posEye = vec3(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
	float dist = length(posEye);
	gl_PointSize = pointRadius * (pointScale / dist);

	gl_TexCoord[0] = gl_MultiTexCoord0;
	gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

	gl_FrontColor = gl_Color;
}
);

char *ParticleRenderer::GfragmentShader =
STRING(
//#version 330 core\n

//layout(location = 0) out float depthOut;
out float depthOut;
out vec3 gPosition;
out vec3 gNormal;


void main()
{
	const vec3 lightDir = vec3(0.577, 0.577, 0.577);
	//float gdepth = texture(depth, gl_TexCoord[0].xy);
	// calculate normal from texture coordinates
	vec3 n;  n.xy = gl_TexCoord[0].xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(n.xy, n.xy);
	if (mag > 1.0)  discard;   // don't draw outside circle
	n.z = sqrt(1.0 - mag);

	// calculate lighting
	float diffuse = max(0.0, dot(lightDir, n));
	//depth = gl_FragCoord.z;
	depthOut = gl_FragCoord.z;
	vec3 gNormal = n;
	gPosition = gl_FragCoord.xyz;
}
);

#define HUE2RGB	\
	"/*int ap[6] = const int[6](2,2,0,0,1,1);\
	int vp[6] = const int[6](0,1,1,2,2,0);\
	int cp[6] = const int[6](1,0,2,1,0,2);*/\
	\
	vec4 hsv2rgb(vec3 hsv)\
	{\
		vec3 rgb;\
		if (hsv.g == 0.0f)  {\
		  if (hsv.b != 0.0f)\
				rgb = hsv.b;  }\
		else\
		{\
			float h = hsv.r * 6.f;		float s = hsv.g;	float v = hsv.b;\
			if (h >= 6.f)	h = 0.f;\
			\
			int i = floor(h);	float f = h - i;\
			\
			float a = 1.f - s;\
			float b = 1.f - s * f;\
			float c = 1.f - s * (1.f - f);\
			\
			/*if (i & 1)  c = b;\
			rgb[ap[i]] = a;  rgb[vp[i]] = 1;  rgb[cp[i]] = c;*/\
			\
			switch (i)	{\
				case 0:  rgb[0] = 1;  rgb[1] = c;  rgb[2] = a;	break;\
				case 1:  rgb[0] = b;  rgb[1] = 1;  rgb[2] = a;	break;\
				case 2:  rgb[0] = a;  rgb[1] = 1;  rgb[2] = c;	break;\
				case 3:  rgb[0] = a;  rgb[1] = b;  rgb[2] = 1;	break;\
				case 4:  rgb[0] = c;  rgb[1] = a;  rgb[2] = 1;	break;\
				case 5:  rgb[0] = 1;  rgb[1] = a;  rgb[2] = b;	break;	}\
			/*rgb *= v;*/\
		}\
		return float4(rgb, 1.f);\
	}"


///  Pixel shader  for rendering points as shaded spheres

char *ParticleRenderer::spherePixelShader[NumProg] = {

STRING(		///  Diffuse
#version 330 core\n

out vec4 FragColor;
in vec2 TexCoords;
uniform sampler2D depth;

void main()
{
	vec3 gdepth = texture(depth, TexCoords).rgb;
	vec3 TexSum = vec3(0, 0, 0);
	for (int i = 0; i < 10; i++) {
		vec2 neighborTexCoord = vec2(TexCoords.x +( 1.0 / 800.0)*i, TexCoords.y + (1.0 / 600.0)*i);
		vec3 gdepthN = texture(depth, neighborTexCoord).rgb;
		TexSum = TexSum + gdepthN;
	}
	vec3 average = vec3(TexSum / 10);
//	float gdepth = texture(depth, gl_TexCoord[0].xy).rgb;
/*	const vec3 lightDir = vec3(0.577, 0.577, 0.577);
	//float gdepth = texture(depth, gl_TexCoord[0].xy);
	// calculate normal from texture coordinates
	vec3 n;  n.xy = TexCoords * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(n.xy, n.xy);
	if (mag > 1.0)  discard;   // don't draw outside circle
	n.z = sqrt(1.0 - mag);
	
	// calculate lighting
	float diffuse = max(0.0, dot(lightDir, n));	

	//gl_FragColor = gl_Color*(fAmbient + fDiffuse * diffuse);
	//gl_FragColor = gl_Color*(fAmbient + fDiffuse * pow(diffuse, fPower));
	//float depth = LinearizeDepth(gl_FragCoord.z) / far;*/
	FragColor = vec4(vec3(average),1.0);
}


),HUE2RGB STRING(	///  Hue
	uniform float fSteps = 0.f;
uniform float fHueDiff;

void main()
{
	vec3 n;  n.xy = gl_TexCoord[0].xy * vec2(2,-2) + vec2(-1,1);
	float mag = dot(n, n);
	if (mag > 1.0)  discard;   // circle

							   // calculate lighting
	const vec3 lightDir = vec3(0.577, 0.577, 0.577);
	n.z = sqrt(1.0 - mag);
	float diffuse = max(0.0, dot(lightDir, n));

	n.x = gl_Color.r;
	if (fSteps > 0.f) { int i = n.x*fSteps;  n.x = i / fSteps; }
	float h = 0.83333f - n.x;
	float s = 1.f;
	if (h < 0.f) { s += h * 6;  h = 0.f; }
	//gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g/*dye*/, 1.f));

	gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g/*dye*/ - diffuse*fHueDiff, 1.f));
}

) };


char *ParticleRenderer::scalePixelShader =
HUE2RGB STRING(		///  Hue Scale
	uniform float fSteps = 0.f;

//uniform float  fBright;
//uniform float  fContrast;

void main()
{
	vec2 n;  n = gl_TexCoord[0].xy;

	if (fSteps > 0.f) { int i = n.x*fSteps;  n.x = i / fSteps; }
	float h = 0.83333f - n.x;
	float s = 1.f;
	if (h < 0.f) { s += h * 6;  h = 0.f; }
	gl_FragColor = hsv2rgb(vec3(h, s, 1.f));
}

);