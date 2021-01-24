#version 130

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

        vec4 temporario = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
		aPos = gl_Vertex.xyz;
        gl_Position = temporario;
        gl_FrontColor = gl_Color;
}