#version 330 core
layout (points) in;
layout (points, max_vertices = 2) out;

in VS_OUT {
 vec4 eyeSpacePos;
 float eyeSpaceRadius;
 float weberNumber;
} gs_in[];  

 out vec4 GeyeSpacePos;
 out float GeyeSpaceRadius;
 out float GweberNumber;

void main() {    
GweberNumber = gs_in[0].weberNumber;
GeyeSpacePos = gs_in[0].eyeSpacePos;
GeyeSpaceRadius = gs_in[0].eyeSpaceRadius;

  /*  gl_Position = gl_in[0].gl_Position; 
	gl_PointSize = gl_in[0].gl_PointSize;
    EmitVertex();*/

	float nIt = round(GweberNumber*10);
	float pointSize = gl_in[0].gl_PointSize;
	float BubbleDensity = 0.5;
	float SprayDensityTH = 0.1;
	float density = 0;

		if(density>BubbleDensity){
		pointSize = pointSize/2.0;
		}
	//	else pointSize = pointSize/(1+rand()%5);

	for(int i = 0; i<nIt; i++){
	if(GweberNumber>0.35){
	gl_Position = gl_in[0].gl_Position + vec4(GweberNumber*0.01*i); 
	gl_PointSize = pointSize;
    EmitVertex();
	}
    }
    EndPrimitive();
}    