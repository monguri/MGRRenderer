#version 430
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	vec4 colorSpecularInt = texture2D(u_texture, v_texCoord);
	gl_FragColor = vec4(colorSpecularInt.rgb, 1.0);
}
