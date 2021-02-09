#version 330

layout( location = 0 ) in vec4 vPosition;
layout( location = 3 ) in float vWeberNumber;
uniform float pointRadius; // point size in world space
uniform float pointScale;  // scale to calculate size in pixels
uniform float SCR_HEIGHT;
uniform mat4 ModelView , Projection;

out vec4 eyeSpacePos;
out float eyeSpaceRadius, weberNumber;

void main() {
	// calculate window-space point size
	eyeSpacePos = ModelView * vPosition;
	vec3 posEye = vec4(ModelView * vec4(vPosition.xyz, 1.0)).xyz;
	float dist = length(posEye);
	eyeSpaceRadius = pointRadius * (pointScale / dist);
	vec4 clipSpacePos = Projection * eyeSpacePos ;

	weberNumber = vWeberNumber ;

        gl_Position = clipSpacePos;
		gl_PointSize = pointRadius * (pointScale / dist);
}