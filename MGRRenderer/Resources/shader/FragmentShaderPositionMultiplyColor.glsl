#version 430
uniform vec3 u_multipleColor;
void main()
{
	gl_FragColor = vec4(u_multipleColor, 1.0);
}
