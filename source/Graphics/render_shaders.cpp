#include "header.h"

#include "render_particles.h"

#define STRING(A) #A
#define STRING2(A, B) #A #B

//** Modified particle rendering shaders (for the G buffer)

char *ParticleRenderer::GvertexShader =
	STRING(#version 130 \n
		uniform float pointRadius; // point size in world space
		uniform float pointScale;  // scale to calculate size in pixels
		out vec3 aPos;
		out vec3 posEye;

		void main() {
			// calculate window-space point size
			posEye = vec4(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0)).xyz;
			float dist = length(posEye);
			gl_PointSize = pointRadius * (pointScale / dist);

			gl_TexCoord[0] = gl_MultiTexCoord0;
			aPos = (gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0)).xyz;

			vec4 temporario = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
			gl_Position = temporario;

			gl_FrontColor = gl_Color;
		});

char *ParticleRenderer::GfragmentShader =
STRING(#version 130\n
uniform float fAmbient; // factors
uniform float fDiffuse;
uniform float fPower;

in vec3 aPos;
in vec3 posEye;
out vec3 gPosition;
out vec3 gNormal;
out vec4 gAlbedoSpec;

void main() {
	const vec3 lightDir = vec3(0.577, 0.577, 0.577);
	// calculate normal from texture coordinates
	vec3 n;
	n.xy = gl_TexCoord[0].xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
	float mag = dot(n.xy, n.xy);
	if (mag > 1.0)
		discard; // don't draw outside circle
	n.z = sqrt(1.0 - mag);

	// calculate lighting
	float diffuse = max(0.0, dot(lightDir, n));

	gNormal = n;
	gPosition = normalize(aPos - posEye);
	gAlbedoSpec = gl_Color; // *(fAmbient + fDiffuse * diffuse);
});

//** Cubemap shaders

char *ParticleRenderer::CubemapVertexShader =
	STRING(#version 130 \n

			   out vec3 TexCoords;
		   void main() {
			   TexCoords = gl_Vertex.xyz;
			   mat4 MVmatrix = mat4(mat3(gl_ModelViewMatrix));
			   vec4 temporario = gl_ProjectionMatrix * MVmatrix * vec4(gl_Vertex.xyz, 1.0);
			   gl_Position = vec4(temporario.xyww);
		   });

char *ParticleRenderer::CubemapFragmentShader =
	STRING(#version 130\n
		in vec3 TexCoords;
		uniform samplerCube skybox;
		out vec4 FragColor;
		void main() {
			FragColor = texture(skybox, TexCoords);
	});

#define HUE2RGB \
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

//** Curvature Flow shaders

char *ParticleRenderer::Curvature_Flow_Vertex_Shader =
STRING(#version 330 core \n
	layout(location = 0) in vec3 aPos;
layout(location = 1) in vec2 aTexCoords;
out vec2 TexCoords;

void main() {
	TexCoords = aTexCoords;
	gl_Position = vec4(aPos, 1.0);
});


char *ParticleRenderer::Curvature_Flow_Fragment_Shader[NumProg] = {
	STRING(#version 330 core\n
			// Diffuse
		uniform float fAmbient; // factors
		uniform float fDiffuse;
		uniform float fPower;
		uniform float SCR_HEIGHT;
		uniform float SCR_WIDTH;

		layout(location = 0) out vec4 FragColor;
		in vec2 TexCoords;
		uniform sampler2D depth;
		uniform sampler2D gPosition;
		uniform sampler2D gNormal;
		//uniform sampler2D gAlbedoSpec;
		uniform samplerCube skybox;
		vec3 currNorm = vec3(0.0);

		float dzdx(int n) {
			if (TexCoords.y >= 1 || TexCoords.x >= 1 || TexCoords.y < 0 || TexCoords.x < 0)
				return 0;
			vec2 x0 = vec2(TexCoords.x + (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			vec2 x1 = vec2(TexCoords.x - (1.0 / SCR_WIDTH) + (1.0 / SCR_WIDTH) * n, TexCoords.y);
			float z0 = texture(depth, x0).r;
			float z1 = texture(depth, x1).r;
			//if (abs(z0 - z1) > 0.01) return 0;
			if (z0 == 1.0 || z1 == 1.0)
				return 0;
			return (z0 - z1) / 2.0;
		}

		//	dz/dy
		float dzdy(int n) {
			if (TexCoords.y >= 1 || TexCoords.x >= 1 || TexCoords.y < 0 || TexCoords.x < 0)
				return 0;
			vec2 y0 = vec2(TexCoords.x, TexCoords.y + (1.0 / SCR_HEIGHT) + (1.0 / SCR_HEIGHT) * n);
			vec2 y1 = vec2(TexCoords.x, TexCoords.y - (1.0 / SCR_HEIGHT) + (1.0 / SCR_HEIGHT) * n);
			float z0 = texture(depth, y0).r;
			float z1 = texture(depth, y1).r;
			//if (abs(z0 - z1) > 0.01) return 0;
			if (z0 == 1.0 || z1 == 1.0)
				return 0;
			return (z0 - z1) / 2.0;
		} float curvature_flow_step(float Z) {
			float height = SCR_HEIGHT;
			float width = SCR_WIDTH;
			float j = TexCoords.y;
			float i = TexCoords.x;

			if (Z > 0)
			{
				float dz_x = dzdx(0);
				float dz_x0 = dzdx(-1);
				float dz_x2 = dzdx(1);
				float dz2x2 = (dz_x2 - dz_x0) / 2.0;

				float dz_y = dzdy(0);
				float dz_y0 = dzdy(-1);
				float dz_y2 = dzdy(1);
				float dz2y2 = (dz_y2 - dz_y0) / 2.0;

				float Cx = i == 0 ? 0 : 2.f / (width * i);
				float Cy = j == 0 ? 0 : 2.f / (height * j);
				float D = Cy * Cy * dz_x * dz_x + Cx * Cx * dz_y * dz_y + Cx * Cx * Cy * Cy * Z * Z;
				float inv_D32 = 1.f / pow(D, 1.5);

				float ky = 4.f / height / height;
				float kx = 4.f / width / width;
				float dD_x = ky * pow(j, -2.f) * 2 * dz_x * dz2x2 +
							 kx * dz_y * dz_y * -2 * pow(i, -3.f) +
							 ky * pow(j, -2.f) * kx * (-2 * pow(i, -3.f) * Z * Z + pow(i, -2.f) * 2 * Z * dz_x);

				float dD_y = kx * pow(i, -2.f) * 2 * dz_y * dz2y2 +
							 ky * dz_x * dz_x * -2 * pow(j, -3.f) +
							 kx * pow(i, -2.f) * ky * (-2 * pow(j, -3.f) * Z * Z + pow(j, -2.f) * 2 * Z * dz_y);

				float Ex = 0.5 * dz_x * dD_x - dz2x2 * D;
				float Ey = 0.5 * dz_y * dD_y - dz2y2 * D;
				if (D != 0)
				{
					float rv_sqrtD = 1.f / sqrt(D);
					currNorm.x = (-Cy * dz_x * rv_sqrtD);
					currNorm.y = (-Cx * dz_y * rv_sqrtD);
					currNorm.z = (Cx * Cy * (1 - Z) * rv_sqrtD);
				}
				//else currNorm = vec3(0);
				return (Cy * Ex + Cx * Ey) * inv_D32 / 2.0;
			}
			else
				return 0;
		}

		void main() {
			vec3 fPos = texture(gPosition, TexCoords).rgb;
			vec3 fNormal = texture(gNormal, TexCoords).rgb;
			//	vec4 fAlbedo = texture(gAlbedoSpec, TexCoords).rgba;
			float Z = texture(depth, TexCoords).r;
			if (Z <= 0.0)
				discard;
			float FlowDepth = curvature_flow_step(Z);
			if (FlowDepth > 1.0)
				FlowDepth = 1.0;
			if (FlowDepth < -1.0)
				FlowDepth = -1.0;

			Z = Z - (FlowDepth * 0.001);
			if (Z > 1)
				Z = 1.0;
			if (Z < 0)
				Z = 0.0;

			const vec3 lightDir = vec3(0.577, 0.577, 0.577);

			float mag = dot(currNorm.xy, currNorm.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle
					//currNorm.z = sqrt(1.0 - mag);

			// calculate lighting
			float diffuse = max(0.0, dot(lightDir, currNorm));
			float ratio = 1.00 / 1.33;
			vec3 R = refract(fPos, normalize(currNorm), ratio);
			vec3 R2 = reflect(fPos, normalize(currNorm));

			//  if(FlowDepth>0) FragColor = fAlbedo*(fAmbient + fDiffuse * pow(diffuse, fPower));
			gl_FragDepth = Z;
			FragColor = (vec4(texture(skybox, R).rgb, 1.0) * 0.6 + vec4(0.2, 0.5, 0.7, 1.0) * 0.1 + vec4(texture(skybox, R2).rgb, 1.0) * 0.3);
			//FragColor = vec4(vec3(Z),1.0);
			//FragColor = vec4(currNorm, 1.0);
		}

		),
	HUE2RGB STRING( ///  Hue
		uniform float fSteps = 0.f;
		uniform float fHueDiff;

		void main() {
			vec3 n;
			n.xy = gl_TexCoord[0].xy * vec2(2, -2) + vec2(-1, 1);
			float mag = dot(n, n);
			if (mag > 1.0)
				discard; // circle

			// calculate lighting
			const vec3 lightDir = vec3(0.577, 0.577, 0.577);
			n.z = sqrt(1.0 - mag);
			float diffuse = max(0.0, dot(lightDir, n));

			n.x = gl_Color.r;
			if (fSteps > 0.f)
			{
				int i = n.x * fSteps;
				n.x = i / fSteps;
			}
			float h = 0.83333f - n.x;
			float s = 1.f;
			if (h < 0.f)
			{
				s += h * 6;
				h = 0.f;
			}
			//gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g/*dye*/, 1.f));

			gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g /*dye*/ - diffuse * fHueDiff, 1.f));
		}

		)};

//** Original shaders

char *ParticleRenderer::vertexShader =
	STRING(#version 130 \n
		uniform float pointRadius; // point size in world space
		uniform float pointScale;  // scale to calculate size in pixels

		void main() {
			// calculate window-space point size
			vec3 posEye = vec3(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0));
			float dist = length(posEye);
			gl_PointSize = pointRadius * (pointScale / dist);

			gl_TexCoord[0] = gl_MultiTexCoord0;
			gl_Position = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);

			gl_FrontColor = gl_Color;
});

char *ParticleRenderer::spherePixelShader[NumProg] = {

	STRING( ///  Diffuse

		uniform float fAmbient; // factors
		uniform float fDiffuse;
		uniform float fPower;

		void main() {
			const vec3 lightDir = vec3(0.577, 0.577, 0.577);

			// calculate normal from texture coordinates
			vec3 n;
			n.xy = gl_TexCoord[0].xy * vec2(2.0, -2.0) + vec2(-1.0, 1.0);
			float mag = dot(n.xy, n.xy);
			if (mag > 1.0)
				discard; // don't draw outside circle
			n.z = sqrt(1.0 - mag);

			// calculate lighting
			float diffuse = max(0.0, dot(lightDir, n));

			//gl_FragColor = gl_Color*(fAmbient + fDiffuse * diffuse);
			gl_FragColor = gl_Color * (fAmbient + fDiffuse * pow(diffuse, fPower));
		}

		),
	HUE2RGB STRING( ///  Hue
		uniform float fSteps = 0.f;
		uniform float fHueDiff;

		void main() {
			vec3 n;
			n.xy = gl_TexCoord[0].xy * vec2(2, -2) + vec2(-1, 1);
			float mag = dot(n, n);
			if (mag > 1.0)
				discard; // circle

			// calculate lighting
			const vec3 lightDir = vec3(0.577, 0.577, 0.577);
			n.z = sqrt(1.0 - mag);
			float diffuse = max(0.0, dot(lightDir, n));

			n.x = gl_Color.r;
			if (fSteps > 0.f)
			{
				int i = n.x * fSteps;
				n.x = i / fSteps;
			}
			float h = 0.83333f - n.x;
			float s = 1.f;
			if (h < 0.f)
			{
				s += h * 6;
				h = 0.f;
			}
			//gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g/*dye*/, 1.f));

			gl_FragColor = hsv2rgb(vec3(h, s - gl_Color.g /*dye*/ - diffuse * fHueDiff, 1.f));
		}

		)};

char *ParticleRenderer::scalePixelShader =
	HUE2RGB STRING( ///  Hue Scale
		uniform float fSteps = 0.f;

		//uniform float  fBright;
		//uniform float  fContrast;

		void main() {
			vec2 n;
			n = gl_TexCoord[0].xy;

			if (fSteps > 0.f)
			{
				int i = n.x * fSteps;
				n.x = i / fSteps;
			}
			float h = 0.83333f - n.x;
			float s = 1.f;
			if (h < 0.f)
			{
				s += h * 6;
				h = 0.f;
			}
			gl_FragColor = hsv2rgb(vec3(h, s, 1.f));
		}

	);

//** Bilateral Filter Shader (not ready)

char *ParticleRenderer::Bilateral_Filter_Fragment_Shader =
STRING(#version 330 \n

in vec2 textureCoords;
out vec4 smoothedParticleDepth;

uniform sampler2D DepthTexture;
uniform float SCR_HEIGHT;
uniform float SCR_WIDTH;
uniform float Kernel[50];
uniform float DomainSigma;
uniform int KernelCenter;

float normpdf(in float x, in float sigma)
{
	return 0.39894f * exp(−0.5 f*x*x / (sigma * sigma) ) / sigma;
}
void main()
{
		vec4 fragmentOriginalValue = texture(DepthTexture, textureCoords);
		if(fragmentOriginalValue.r == 0.0f)
		{
			discard;
		}
		float fragmentFinalValue = 0.0f;
		float z = 0.0f;
		float currentValue;
		float smoothingFactor;
		float bZ = 1.0 / normpdf(0.0, DomainSigma);
		//Apply BF
			for(int i = −KernelCenter; i <= KernelCenter; ++i)
			{
				for(int j = −KernelCenter; j <= KernelCenter; ++j)
				{
				currentValue = texture(DepthTexture, (textureCoords + (vec2(float(i) / SCR_WIDTH, float(j)) / SCR_HEIGHT))).r;
				smoothingFactor = normpdf(currentValue − fragmentOriginalValue.r, DomainSigma)*bZ* Kernel[KernelCenter + i] * Kernel[KernelCenter + j];
					z += smoothingFactor;
					fragmentFinalValue += smoothingFactor * currentValue;
				}
			}

	float fragmentDepth = fragmentFinalValue / z;
	smoothedParticleDepth = vec4(vec3(fragmentDepth), fragmentOriginalValue.a);
	glFragDepth = fragmentDepth;
});