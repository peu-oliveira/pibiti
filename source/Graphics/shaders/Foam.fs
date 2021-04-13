#version 330 core

in vec4 GeyeSpacePos ;
in float GeyeSpaceRadius, GweberNumber ;
out vec4 particleDepth ;
uniform mat4 Projection ;
uniform float SCR_HEIGHT;
uniform float SCR_WIDTH;

void main( ){

float Near = 0.1f ,far =1000.0f;
float foamThreshold = 0.05f; //values between 0.0 and 1.0?

if ( GweberNumber >= foamThreshold )
{
//discard ;
}
vec3 normal;
normal.xy = ( gl_PointCoord - 0.5f ) * 2.0f ;
float dist = length( normal ) ;
float sizeAndAlphaFactor = clamp ( GweberNumber /foamThreshold , 0.0f , 1.0f ) ;
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
// C al c ul a t ef ragmen t p o s i t i o n i n eye space , p r o j e c t t of i n d depth
vec4 fragPos = vec4 ( GeyeSpacePos.xyz + normal * GeyeSpaceRadius / SCR_HEIGHT ,1.0 ) ;
vec4 clipspacePos = Projection *fragPos ;
// Se t up output
float deviceDepth = clipspacePos.z / clipspacePos.w ;
float fragDepth = ( ( 2.0f * Near ) / (far + Near - ( deviceDepth * 0.5 + 0.5 ) * (far - Near ) ) ) ;
particleDepth = vec4 ( vec3 (GweberNumber ) , 1.0 - sizeAndAlphaFactor ) ;
if ( GweberNumber >= foamThreshold )
{
particleDepth = vec4 ( vec3 (GweberNumber ) , 0.0f ) ; //"discard" outputing 1.0
}
particleDepth = vec4(1.0);
gl_FragDepth = fragDepth ;
}
