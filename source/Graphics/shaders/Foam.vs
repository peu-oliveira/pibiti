#version 330

layout( location = 0 ) in vec4 vPosition;
layout( location = 3 ) in vec3 vWeberNumber;
uniform float pointRadius; // point size in world space
uniform float pointScale;  // scale to calculate size in pixels
uniform float SCR_HEIGHT;
uniform mat4 ModelView , Projection;

out VS_OUT {
 vec4 eyeSpacePos;
 float eyeSpaceRadius;
 vec3 weberNumber;
} vs_out;

//out float searchRadius;
 vec4 eyeSpacePos;

void main() {
	// calculate window-space point size
	eyeSpacePos = ModelView * vPosition;
    vs_out.eyeSpacePos = eyeSpacePos;
	vec3 posEye = vec4(ModelView * vec4(vPosition.xyz, 1.0)).xyz;
	float dist = length(posEye);
	vs_out.eyeSpaceRadius = pointRadius * (pointScale / dist);
	vec4 clipSpacePos = Projection * eyeSpacePos ;

	vs_out.weberNumber = vWeberNumber ;

        gl_Position = clipSpacePos;
		gl_PointSize = pointRadius * (pointScale / dist);

		//Foam
		/*

		//Calculate radius for diferent type of particles
		if(vWeberNumber<SprayDensityTH){
		eyeSpaceRadius = eyeSpaceRadius/(1+rand()%5);
		}
		if(vWeberNumber>BubbleDensity){
		eyeSpaceRadius = eyeSpaceRadius/2.0;
		}

		//Calculate searchRadius
		searchRadius = eyeSpaceRadius / (tan(alpha/2)*abs(z)); //alpha = field of view of the camera and z = distance of the sprite to the camera
		*/
}