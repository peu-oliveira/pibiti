#version 130

//layout( location =1 ) in float vWeberNumber;
uniform float pointRadius; // point size in world space
uniform float pointScale;  // scale to calculate size in pixels
uniform float SCR_HEIGHT;
out vec3 aPos;
out vec3 posEye;
out vec4 eyeSpacePos;
out float eyeSpaceRadius;

void main() {
	// calculate window-space point size
	eyeSpacePos = gl_ModelViewMatrix * gl_Vertex;
	posEye = vec4(gl_ModelViewMatrix * vec4(gl_Vertex.xyz, 1.0)).xyz;
	float dist = length(posEye);
	eyeSpaceRadius = pointRadius * (pointScale / dist);
	gl_PointSize = pointRadius * (pointScale / dist);

	gl_TexCoord[0] = gl_MultiTexCoord0;

        vec4 temporario = gl_ModelViewProjectionMatrix * vec4(gl_Vertex.xyz, 1.0);
		aPos = gl_Vertex.xyz;
        gl_Position = temporario;
        gl_FrontColor = gl_Color;
}