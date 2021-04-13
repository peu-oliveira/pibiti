#version 330 core

in vec4 eyeSpacePos ;
in float eyeSpaceRadius , weberNumber ;
in float SearchRadius;
in float density;

out vec4 FoamThickness;

layout(location = 0) out vec4 Thickness;

uniform mat4 Projection ;
uniform float SCR_HEIGHT;
uniform float SCR_WIDTH;
uniform sampler2D depth;

float FallOffFunc(float x, float b, float n, float m){
	float falloff;//?
	if(x/b<1){
	falloff = pow((1-pow(x/b,n)),m);
	}
	else falloff = 0;
	return falloff;
}

void main( ){
float b,n,m,x;
if(spray){
b=1.0;
m=1.15;
n=1.0;
}
if(foam){
b=1.0;
m=2.25;
n=1.0;
}
if(bubble){
b=1.0;
m=2.0;
n=1.0;
}

FoamThickness = vec4(FallOffFunc(x,b,n,m));

}