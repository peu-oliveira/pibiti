#version 330 core

in vec2 TexCoords;
layout (location = 0) out vec4 smoothedParticleDepth;
layout(location = 1) out vec3 fPosition;
layout(location = 2) out vec4 thickness;

uniform sampler2D DepthTexture;
uniform float SCR_HEIGHT;
uniform float SCR_WIDTH;
uniform float Kernel[15];
uniform float DomainSigma;
uniform int KernelCenter;
uniform sampler2D gPosition;
uniform sampler2D particleThickness;

float normpdf(in float x, in float sigma)
{
	return 0.39894f * exp(-0.5f*x*x / (sigma * sigma) ) / sigma;
}
void main()
{
		vec4 fragmentOriginalValue = texture(DepthTexture, TexCoords);
		fPosition = texture(gPosition, TexCoords).xyz;
		thickness = texture(particleThickness, TexCoords);
		if(fragmentOriginalValue.r == 0.0f || fragmentOriginalValue.r == 1.0f)
		{
			discard;
		}
		float fragmentFinalValue = 0.0f;
		float z = 0.0f;
		float currentValue;
		float smoothingFactor;
		float bZ = 1.0 / normpdf(0.0, DomainSigma);
		//Apply BF
			for(int i = -KernelCenter; i <= KernelCenter; ++i)
			{
				for(int j = -KernelCenter; j <= KernelCenter; ++j)
				{
				currentValue = texture(DepthTexture, (TexCoords + (vec2(float(i) / SCR_WIDTH, float(j)) / SCR_HEIGHT))).r;
				smoothingFactor = normpdf(currentValue - fragmentOriginalValue.r, DomainSigma)*bZ* Kernel[KernelCenter + i] * Kernel[KernelCenter + j];
				//smoothingFactor = normpdf(currentValue - fragmentOriginalValue.r, DomainSigma)*exp(-Fi*Fi*0.1)* exp(-Fj*Fj*0.1);
					z += smoothingFactor;
					fragmentFinalValue += smoothingFactor * currentValue;
				}
			}

	float fragmentDepth = fragmentFinalValue / z;
	//smoothedParticleDepth = vec4(vec3(fragmentDepth), fragmentOriginalValue.a);
	smoothedParticleDepth = vec4(fragmentOriginalValue);
	gl_FragDepth = fragmentDepth;

}