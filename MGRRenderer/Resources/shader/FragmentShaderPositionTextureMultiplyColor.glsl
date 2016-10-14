const char* FRAGMENT_SHADER_POSITION_TEXTURE_MULTIPLY_COLOR = STRINGIFY(
uniform sampler2D u_texture;
uniform vec3 u_multipleColor;
varying vec2 v_texCoord;
void main()
{
	gl_FragColor = texture2D(u_texture, v_texCoord) * vec4(u_multipleColor, 1.0); // テクスチャ番号は0のみに対応
}
);
