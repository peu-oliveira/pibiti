#version 130

out vec3 TexCoords;

void main() {

 TexCoords = gl_Vertex.xyz;
 mat4 MVmatrix = mat4(mat3(gl_ModelViewMatrix));
 vec4 temporario = gl_ProjectionMatrix * MVmatrix * vec4(gl_Vertex.xyz, 1.0);
 gl_Position = vec4(temporario.xyww);

}