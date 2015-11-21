#include "Sprite3D.h"
#include "ObjLoader.h"
#include "Image.h"
#include "Director.h"

namespace mgrrenderer
{

Sprite3D::Sprite3D() : _texture(nullptr)
{
}

Sprite3D::~Sprite3D()
{
	glBindTexture(GL_TEXTURE_2D, 0);
	if (_texture)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Sprite3D::initWithModel(const Vec3& position, float scale, const std::string& filePath)
{
	//TODO: _position�ɂ�郏�[���h���W�ւ̕ϊ��������Ȃ��ƁB�B

	if (filePath.substr(filePath.length() - 4, 4) == ".obj")
	{
		std::vector<ObjLoader::MeshData> meshList;
		std::vector<ObjLoader::MaterialData> materialList;

		const std::string& err = ObjLoader::loadObj(filePath, meshList, materialList);
		if (!err.empty())
		{
			printf(err.c_str());
			return false;
		}

		_vertices.clear();

		// materialList�͌��󖳎�
		// TODO:��������MeshData�͂��̎��_�Ń}�e���A�����Ƃɂ܂Ƃ܂��ĂȂ��̂ł́HfaceGroup������܂Ƃ܂��Ă�H
		// ���܂Ƃ܂��Ă�B�������A����̓}�e���A���͈��ނƂ����O��ł�����
		// �{���́Astd::vector<std::vector<Position3DTextureCoordinates>> �������o�ϐ��ɂȂ��ĂāA�}�e���A�����Ƃɐ؂�ւ��ĕ`�悷��
		// �e�N�X�`�����{���͐؂�ւ��O�񂾂���setTexture���ă��\�b�h����������ȁB�B
		assert(meshList.size() == 1);
		const ObjLoader::MeshData& mesh = meshList[0];
		_vertices = mesh.vertices;
		_indices = mesh.indices;

		size_t numVertex = _vertices.size();
		for (int i = 0; i < numVertex; ++i)
		{
			// ���[�J�����W���烏�[���h���W�ɍ��W�ϊ�
			//TODO: �����ƍs��ł�肽��
			Position3DTextureCoordinates vertex = _vertices[i];
			vertex.position *= scale;
			vertex.position += position;
			_vertices[i] = vertex;
		}
	}
	else
	{
		// TODO:�܂�c3b�ɂ͖��Ή�
		assert(false);
	}

	_glData = createOpenGLProgram(
		// vertex shader
		"attribute mediump vec4 attr_position;"
		"attribute mediump vec2 attr_texCoord;"
		"varying mediump vec2 vary_texCoord;"
		"uniform mediump mat4 unif_view_mat;"
		"uniform mediump mat4 unif_proj_mat;"
		"void main()"
		"{"
		"	gl_Position = unif_proj_mat * unif_view_mat * attr_position;"
		"	vary_texCoord = attr_texCoord;"
		"}"
		,
		// fragment shader
		"uniform sampler2D texture;"
		"varying mediump vec2 vary_texCoord;"
		"void main()"
		"{"
		"	gl_FragColor = texture2D(texture, vary_texCoord);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
		"}"
		);

	_glData.attributeTextureCoordinates = glGetAttribLocation(_glData.shaderProgram, "attr_texCoord");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.attributeTextureCoordinates < 0)
	{
		return false;
	}

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "texture");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.uniformTexture < 0)
	{
		return false;
	}

	return true;
}

void Sprite3D::setTexture(const std::string& filePath)
{
	// Texture�����[�h���Apng��jpeg�𐶃f�[�^�ɂ��AOpenGL�ɂ�����d�g�݂����˂΁B�BSprite�̃\�[�X���������Ƃ����B
	Image image; // Image��CPU���̃��������g���Ă���̂ł��̃X�R�[�v�ŉ������Ă��悢���̂�����X�^�b�N�Ɏ��
	image.initWithFilePath(filePath);

	_texture = new Texture(); // Texture��GPU���̃��������g���Ă�̂ŉ�������ƍ���̂Ńq�[�v�ɂƂ�
	_texture->initWithImage(image);
}

void Sprite3D::render()
{
	// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeTextureCoordinates);
	assert(glGetError() == GL_NO_ERROR);

	glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DTextureCoordinates), (GLvoid*)&_vertices[0].position);
	assert(glGetError() == GL_NO_ERROR);
	glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
	assert(glGetError() == GL_NO_ERROR);
	// TODO:�C���f�b�N�X���g����glDrawElements������

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	assert(glGetError() == GL_NO_ERROR);
	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, &_indices[0]);
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer
