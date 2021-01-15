#version 330 \n

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
	return 0.39894f * exp(-0.5f*x*x / (sigma * sigma) ) / sigma;
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
}