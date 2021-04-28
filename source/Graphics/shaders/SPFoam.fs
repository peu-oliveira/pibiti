#version 330 core

in vec4 eyeSpacePos ;
in float eyeSpaceRadius , weberNumber ;
in float SearchRadius;

layout(location = 0) out vec4 particleDepth;
layout(location = 1) out SearchRadius;
layout(location = 2) out vec3 WSNormal;

uniform mat4 Projection ;
uniform float SCR_HEIGHT;
uniform float SCR_WIDTH;

void main( ){

float Near = 0.1f ,far =1000.0f;
float foamThreshold = 0.05f; //values between 0.0 and 1.0?

if ( weberNumber >= foamThreshold )
{
//discard ;
}
vec3 normal;
normal.xy = ( gl_PointCoord - 0.5f ) * 2.0f ;
float dist = length( normal ) ;
float sizeAndAlphaFactor = clamp ( weberNumber /foamThreshold , 0.0f , 1.0f ) ;
// Di sc a rd p i x e l s o u t si d e the s p h e r e
if ( dist > 1.0f * sizeAndAlphaFactor )
{
//particleDepth = vec4(0.0);
discard ;
}
// Se t up r e s t of normal
normal.z = sqrt( 1.0f - dist ) ;
normal.y = -normal.y ;
normal = normalize( normal ) ;


//Calculate w cordinate in point sprite
float u = gl_PointCoord.x;
float v = gl_PointCoord.y;
float w = sqrt(1-u*u-v*v);
vec3 SphericalDepth = vec3(u,v,w);

WSNormal = vec3(u,v,w)*transpose(normalMatrix);

vec4 fragPos = vec4 ( eyeSpacePos.xyz + normal * eyeSpaceRadius / SCR_HEIGHT ,1.0 ) ;
vec4 clipspacePos = Projection *fragPos ;
// Se t up output
float deviceDepth = clipspacePos.z / clipspacePos.w ;
float fragDepth = ( ( 2.0f * Near ) / (far + Near - ( deviceDepth * 0.5 + 0.5 ) * (far - Near ) ) ) ;
particleDepth = vec4 ( vec3 (weberNumber ) , 1.0 - sizeAndAlphaFactor ) ;
if ( weberNumber >= foamThreshold )
{
particleDepth = vec4 ( vec3 (weberNumber ) , 0.0f ) ; //"discard" outputing 1.0
}
//particleDepth = vec4(fragDepth);
gl_FragDepth = fragDepth + w*eyeSpaceRadius;
}