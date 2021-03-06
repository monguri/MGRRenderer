// ディファードレンダリングに使うので、マルチレンダーターゲットのためにGLSL4.3.0で書く
#version 430

in vec4 a_position;
in vec4 a_normal;
in vec2 a_texCoord;

out vec4 v_position;
out vec4 v_normal;
out vec2 v_texCoord;

uniform mat4 u_modelMatrix;
uniform mat4 u_viewMatrix;
uniform mat4 u_projectionMatrix;
uniform mat4 u_normalMatrix;

void main()
{
	v_position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;
	v_normal = vec4(normalize((u_normalMatrix * a_normal).xyz), 1.0);
	v_texCoord = a_texCoord;
	v_texCoord.y = 1.0 - v_texCoord.y; // objの事情によるもの
	gl_Position = v_position;
}
