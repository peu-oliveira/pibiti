#version 130
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
}