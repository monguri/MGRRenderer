#version 430
uniform sampler2D u_texture;

varying vec2 v_texCoord;

void main()
{
	float specPower = texture2D(u_texture, v_texCoord).x;
	gl_FragColor = vec4(specPower, specPower, specPower, 1.0);
}
