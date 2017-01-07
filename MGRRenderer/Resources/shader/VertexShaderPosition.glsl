#version 430
attribute vec4 a_position;
uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
void main()
{
	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;
}
