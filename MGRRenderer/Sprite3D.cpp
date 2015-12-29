#include "Sprite3D.h"
#include "ObjLoader.h"
#include "C3bLoader.h"
#include "Image.h"
#include "Director.h"

namespace mgrrenderer
{

Sprite3D::Sprite3D() :
_texture(nullptr),
_meshDatas(nullptr),
_nodeDatas(nullptr),
_perVertexByteSize(0),
_animationDatas(nullptr),
_currentAnimation(nullptr),
_loopAnimation(false),
_elapsedTime(0.0f),
_isCpuMode(false)
{
}

Sprite3D::~Sprite3D()
{
	glBindTexture(GL_TEXTURE_2D, 0);

	_currentAnimation = nullptr;

	if (_animationDatas != nullptr)
	{
		delete _animationDatas;
		_animationDatas = nullptr;
	}

	if (_nodeDatas != nullptr)
	{
		delete _nodeDatas;
		_nodeDatas = nullptr;
	}

	if (_meshDatas != nullptr)
	{
		delete _meshDatas;
		_meshDatas = nullptr;
	}

	if (_texture != nullptr)
	{
		delete _texture;
		_texture = nullptr;
	}
}

bool Sprite3D::initWithModel(const std::string& filePath)
{
	_isObj = false;
	_isC3b = false;

	const std::string& ext = filePath.substr(filePath.length() - 4, 4);
	if (ext == ".obj")
	{
		_isObj = true;

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
	}
	else if (ext == ".c3t")
	{
		_isC3b = true; // TODO:���̃t���O�����͔��ɂ�����

		_meshDatas = new (std::nothrow)C3bLoader::MeshDatas();
		C3bLoader::MaterialDatas* materialDatas = new (std::nothrow)C3bLoader::MaterialDatas();
		_nodeDatas = new (std::nothrow)C3bLoader::NodeDatas();
		_animationDatas = new (std::nothrow)C3bLoader::AnimationDatas();
		const std::string& err = C3bLoader::loadC3t(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		if (!err.empty())
		{
			printf(err.c_str());
			return false;
		}

		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		_indices = meshData->subMeshIndices[0];

		_perVertexByteSize = 0;

		for (C3bLoader::MeshVertexAttribute attrib : meshData->attributes)
		{
			_perVertexByteSize += attrib.attributeSizeBytes;
		}

		C3bLoader::MaterialData* materialData = materialDatas->materialDatas[0];
		const C3bLoader::TextureData& texture = materialData->textures[0];
		setTexture(texture.fileName);

		delete materialDatas;
	}
	else
	{
		// TODO:�܂�c3b�ɂ͖��Ή�
		assert(false);
	}

	if (_isObj)
	{
		_glData = createOpenGLProgram(
			// vertex shader
			// ModelData�����g��Ȃ��ꍇ
			"attribute vec4 attr_position;"
			"attribute vec2 attr_texCoord;"
			"varying vec2 vary_texCoord;"
			"uniform mat4 unif_modelMatrix;"
			"uniform mat4 unif_viewMatrix;"
			"uniform mat4 unif_projectionMatrix;"
			"void main()"
			"{"
			"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * attr_position;"
			"	vary_texCoord = attr_texCoord;"
			"}"
			,
			// fragment shader
			"uniform sampler2D unif_texture;"
			"varying vec2 vary_texCoord;"
			"void main()"
			"{"
			"	gl_FragColor = texture2D(unif_texture, vary_texCoord);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
			"}"
			);
	}
	else if (_isC3b)
	{
		if (_isCpuMode)
		{
			_glData = createOpenGLProgram(
				// vertex shader
				// ModelData�����g��Ȃ��ꍇ
				"attribute vec4 attr_position;"
				"attribute vec2 attr_texCoord;"
				"varying vec2 vary_texCoord;"
				"uniform mat4 unif_modelMatrix;"
				"uniform mat4 unif_viewMatrix;"
				"uniform mat4 unif_projectionMatrix;"
				"void main()"
				"{"
				"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * attr_position;"
				"	vary_texCoord = attr_texCoord;"
				"	vary_texCoord.y = 1.0 - vary_texCoord.y;" // c3b�̎���ɂ�����
				"}"
				,
				// fragment shader
				"uniform sampler2D unif_texture;"
				"varying vec2 vary_texCoord;"
				"void main()"
				"{"
				"	gl_FragColor = texture2D(unif_texture, vary_texCoord);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
				"}"
				);
		}
		else
		{
			_glData = createOpenGLProgram(
				// vertex shader
				// ModelData�����g��Ȃ��ꍇ
				//"attribute vec4 attr_position;"
				//"attribute vec2 attr_texCoord;"
				//"varying vec2 vary_texCoord;"
				//"uniform mat4 unif_modelMatrix;"
				//"uniform mat4 unif_viewMatrix;"
				//"uniform mat4 unif_projectionMatrix;"
				//"void main()"
				//"{"
				//"	gl_Position = unif_projectionMatrix * unif_viewMatrix * attr_position;"
				//"	vary_texCoord = attr_texCoord;"
				//"	vary_texCoord.y = 1.0 - vary_texCoord.y;"
				//"}"
				//// �A�j���[�V�������g���ꍇ
				"attribute vec3 attr_position;" // ���ꂪvec3�ɂȂ��Ă���̂ɒ��� TODO:�Ȃ��Ȃ̂��H
				"attribute vec2 attr_texCoord;"
				"attribute vec4 attr_blendWeight;"
				"attribute vec4 attr_blendIndex;"
				""
				"const int SKINNING_JOINT_COUNT = 60;" // TODO:�Ȃ�60�܂łȂ̂��H
				""
				"uniform mat4 unif_modelMatrix;"
				"uniform mat4 unif_viewMatrix;"
				"uniform mat4 unif_projectionMatrix;"
				"uniform mat4 unif_matrixPalette[SKINNING_JOINT_COUNT];"
				""
				"varying vec2 vary_texCoord;"
				""
				"vec4 getPosition()"
				"{"
				"	mat4 skinMatrix = unif_matrixPalette[int(attr_blendIndex[0])] * attr_blendWeight[0];"
				""
				"	if (attr_blendWeight[1] > 0.0)"
				"	{"
				"		skinMatrix += unif_matrixPalette[int(attr_blendIndex[1])] * attr_blendWeight[1];"
				""
				"		if (attr_blendWeight[2] > 0.0)"
				"		{"
				"			skinMatrix += unif_matrixPalette[int(attr_blendIndex[2])] * attr_blendWeight[2];"
				""
				"			if (attr_blendWeight[3] > 0.0)"
				"			{"
				"				skinMatrix += unif_matrixPalette[int(attr_blendIndex[3])] * attr_blendWeight[3];"
				"			}"
				"		}"
				"	}"
				""
				"	vec4 position = vec4(attr_position, 1.0);"
				"	vec4 skinnedPosition = skinMatrix * position;"
				"	skinnedPosition.w = 1.0;"
				"	return skinnedPosition;"
				"}"
				""
				"void main()"
				"{"
				"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * getPosition();"
				"	vary_texCoord = attr_texCoord;"
				"	vary_texCoord.y = 1.0 - vary_texCoord.y;" // c3b�̎���ɂ�����
				"}"
				//
				// cocos�̂��
				//
				//"attribute vec3 attr_position;"
				//"attribute vec4 attr_blendWeight;"
				//"attribute vec4 attr_blendIndex;"
				//""
				//"attribute vec2 attr_texCoord;"
				//""
				//"const int SKINNING_JOINT_COUNT = 60;"
				//""
				//"uniform vec4 unif_matrixPalette[SKINNING_JOINT_COUNT * 3];"
				//""
				//"varying vec2 vary_texCoord;"
				//"uniform mat4 unif_modelMatrix;"
				//"uniform mat4 unif_viewMatrix;"
				//"uniform mat4 unif_projectionMatrix;"
				//""
				//"vec4 getPosition()"
				//"{"
				//"	float blendWeight = attr_blendWeight[0];"
				//""
				//"	int matrixIndex = int(attr_blendIndex[0]) * 3;"
				//"	vec4 matrixPalette1 = unif_matrixPalette[matrixIndex] * blendWeight;"
				//"	vec4 matrixPalette2 = unif_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"	vec4 matrixPalette3 = unif_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//""
				//"	blendWeight = attr_blendWeight[1];"
				//"	if (blendWeight > 0.0)"
				//"	{"
				//"		matrixIndex = int(attr_blendIndex[1]) * 3;"
				//"		matrixPalette1 += unif_matrixPalette[matrixIndex] * blendWeight;"
				//"		matrixPalette2 += unif_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"		matrixPalette3 += unif_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//"		blendWeight = attr_blendWeight[2];"
				//"		if (blendWeight > 0.0)"
				//"		{"
				//"			matrixIndex = int(attr_blendIndex[2]) * 3;"
				//"			matrixPalette1 += unif_matrixPalette[matrixIndex] * blendWeight;"
				//"			matrixPalette2 += unif_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"			matrixPalette3 += unif_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//"			blendWeight = attr_blendWeight[3];"
				//"			if (blendWeight > 0.0)"
				//"			{"
				//"				matrixIndex = int(attr_blendIndex[3]) * 3;"
				//"				matrixPalette1 += unif_matrixPalette[matrixIndex] * blendWeight;"
				//"				matrixPalette2 += unif_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"				matrixPalette3 += unif_matrixPalette[matrixIndex + 2] * blendWeight;"
				//"			}"
				//"		}"
				//"	}"
				//""
				//"	vec4 _skinnedPosition;"
				//"	vec4 postion = vec4(attr_position, 1.0);"
				//"	_skinnedPosition.x = dot(postion, matrixPalette1);"
				//"	_skinnedPosition.y = dot(postion, matrixPalette2);"
				//"	_skinnedPosition.z = dot(postion, matrixPalette3);"
				//"	_skinnedPosition.w = postion.w;"
				//""
				//"	return _skinnedPosition;"
				//"}"
				//""
				//"void main()"
				//"{"
				//"	vec4 position = getPosition();"
				//"	gl_Position = unif_projectionMatrix * unif_viewMatrix * unif_modelMatrix * position;"
				//""
				//"	vary_texCoord = attr_texCoord;"
				//"	vary_texCoord.y = 1.0 - vary_texCoord.y;"
				//"}"
				,
				// fragment shader
				"uniform sampler2D unif_texture;"
				"varying vec2 vary_texCoord;"
				"void main()"
				"{"
				"	gl_FragColor = texture2D(unif_texture, vary_texCoord);" // �e�N�X�`���ԍ���0�݂̂ɑΉ�
				"}"
				);
		}
	}

	_glData.attributeTextureCoordinates = glGetAttribLocation(_glData.shaderProgram, "attr_texCoord");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.attributeTextureCoordinates < 0)
	{
		return false;
	}

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "unif_texture");
	if (glGetError() != GL_NO_ERROR)
	{
		return false;
	}

	if (_glData.uniformTexture < 0)
	{
		return false;
	}

	if (_isC3b && !_isCpuMode)
	{
		_glData.uniformSkinMatrixPalette = glGetUniformLocation(_glData.shaderProgram, "unif_matrixPalette");
		if (glGetError() != GL_NO_ERROR)
		{
			return false;
		}

		if (_glData.uniformSkinMatrixPalette < 0)
		{
			return false;
		}
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

void Sprite3D::startAnimation(const std::string& animationName, bool loop /* = false*/)
{
	_elapsedTime = 0.0f;
	_currentAnimation = _animationDatas->animations[animationName];
	_loopAnimation = loop;
}

void Sprite3D::stopAnimation()
{
	_currentAnimation = nullptr;
}

C3bLoader::NodeData* Sprite3D::findJointByName(const std::string& jointName, const std::vector<C3bLoader::NodeData*> children)
{
	for (C3bLoader::NodeData* child : children)
	{
		if (child->id == jointName)
		{
			// ���������炻���Ԃ��B���[�v���I���B
			return child;
		}
		else if (child->children.size() > 0)
		{
			// �����炸�A�q������Ȃ�q��T���ɍs��
			C3bLoader::NodeData* findResult = findJointByName(jointName, child->children);
			if (findResult != nullptr)
			{
				// ���������炻���Ԃ��B���[�v���I���B
				return findResult;
			}
		}
	}

	return nullptr;
}

void Sprite3D::update(float dt)
{
	if (!_isC3b) {
		return;
	}

	float t = 0; // 0<=t<=1�̃A�j���[�V�����⊮�p�����[�^
	if (_currentAnimation != nullptr)
	{
		_elapsedTime += dt;
		if (_loopAnimation && _elapsedTime > _currentAnimation->totalTime)
		{
			_elapsedTime = 0.0f;
		}

		t = _elapsedTime / _currentAnimation->totalTime;
	}

	_matrixPalette.clear();

	// �A�j���[�V�������s���BAnimate3D::update���Q�l�� C3bLoader::AnimationData�̎g�����Ȃ����̂�
	size_t numSkinJoint = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones.size();
	assert(numSkinJoint == _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose.size());

	// ��Ɋe�W���C���g�̃A�j���[�V�����s����쐬����
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			printf("nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");
		}
		assert(node != nullptr);

		// ��U�A�A�j���[�V�����g��Ȃ����ɏ�����
		node->animatedTransform = Mat4::ZERO;

		if (_currentAnimation != nullptr && _currentAnimation->translationKeyFrames.find(jointName) != _currentAnimation->translationKeyFrames.end())
		{
			// �A�j���[�V�������ŁA�f�[�^�ɂ��̃{�[���̃L�[�t���[����񂪂������Ƃ��͂����炩�������s��ŏ㏑������
			const Vec3& translation = _currentAnimation->evaluateTranslation(jointName, t);
			const Quaternion& rotation = _currentAnimation->evaluateRotation(jointName, t);
			const Vec3& scale = _currentAnimation->evaluateScale(jointName, t);

			node->animatedTransform = Mat4::createTransform(translation, rotation, scale);
		}
	}

	// ���ɃW���C���g�̃}�g���b�N�X�p���b�g�����ׂċ��߂�
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		// �L�[�t���[����񂪂Ȃ�������ANodeDatas::skeletons::transform�̕��̃c���[����ċA�I�ɃW���C���g�ʒu���W�ϊ��z����v�Z����
		// �܂��͑Ή�����{�[����T���A�ݒ肳��Ă���s����擾����
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			printf("nodes�̕��ɂ��������̂�skeletons����{�[�����Ō����������Ȃ��B");
		}
		assert(node != nullptr);

		Mat4 transform = (node->animatedTransform != Mat4::ZERO) ? node->animatedTransform : node->transform;

		// �e�����[�g�܂ł����̂ڂ��ă��[���h�s����v�Z����
		while (node->parent != nullptr)
		{
			// TODO:�e�̃{�[�����A�j���[�V�������Ă�\�����������߂ɁANodeData�ɂ������Matrix��������Ƃ��������ȕ��@�B�{����Skeleton3D��Bone3D�݂����ɕʂ̊K�w�\���f�[�^��ێ�������������
			const Mat4& parentTransform = (node->parent->animatedTransform != Mat4::ZERO) ? node->parent->animatedTransform : node->parent->transform;
			transform = parentTransform * transform;
			node = node->parent;
		}

		// �{�[���̔z�u�s��
		const Mat4& invBindPose = _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose[i];//TODO: nodes���ɗv�f�͈�Aparts���ɂ�������ł��邱�Ƃ�O��ɂ��Ă���

		Mat4 matrix = transform * invBindPose;
		//matrix.transpose(); // TODO:�]�u�͂��������̂��H �V�F�[�_�ł�Mv���A���ۂ͍s��x��x�N�g���łȂ��AvtMt�̂悤�ɍs�x�N�g��x�s��̓]�u�ƍl����Ɠ]�u�͂��������B �����ە\�������Ă݂�Ɛ������Ȃ����ۂ��B���[��B�B�B
		_matrixPalette.push_back(matrix);
		//_matrixPalette.push_back(Vec4(matrix.m[0][0],matrix.m[1][0],matrix.m[2][0],matrix.m[3][0]));
		//_matrixPalette.push_back(Vec4(matrix.m[0][1],matrix.m[1][1],matrix.m[2][1],matrix.m[3][1]));
		//_matrixPalette.push_back(Vec4(matrix.m[0][2],matrix.m[1][2],matrix.m[2][2],matrix.m[3][2]));
	}
}

void Sprite3D::render()
{
	// cocos2d-x��TriangleCommand���s���Ă�`������ȁB�B�e�N�X�`���o�C���h��Texture2D�ł���Ă�̂ɑ��v���H
	glUseProgram(_glData.shaderProgram);
	assert(glGetError() == GL_NO_ERROR);

	if (_isC3b && !_isCpuMode) {
		assert(_matrixPalette.size() > 0);
		glUniformMatrix4fv(_glData.uniformSkinMatrixPalette, _matrixPalette.size(), GL_FALSE, (GLfloat*)(_matrixPalette[0].m));
		//glUniform4fv(_glData.uniformSkinMatrixPalette, _matrixPalette.size(), (GLfloat*)(&_matrixPalette[0]));
	}

	glUniformMatrix4fv(_glData.uniformModelMatrix, 1, GL_FALSE, (GLfloat*)getModelMatrix().m);
	glUniformMatrix4fv(_glData.uniformViewMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getViewMatrix().m);
	glUniformMatrix4fv(_glData.uniformProjectionMatrix, 1, GL_FALSE, (GLfloat*)Director::getCamera().getProjectionMatrix().m);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::POSITION);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_WEIGHT);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray((GLuint)AttributeLocation::BLEND_INDEX);
	assert(glGetError() == GL_NO_ERROR);

	glEnableVertexAttribArray(_glData.attributeTextureCoordinates);
	assert(glGetError() == GL_NO_ERROR);

	if (_isObj)
	{
		glVertexAttribPointer((GLuint)AttributeLocation::POSITION, 3, GL_FLOAT, GL_FALSE, sizeof(Position3DTextureCoordinates), (GLvoid*)&_vertices[0].position);
		assert(glGetError() == GL_NO_ERROR);
		glVertexAttribPointer(_glData.attributeTextureCoordinates, 2, GL_FLOAT, GL_FALSE, sizeof(Position3DTextureCoordinates), (GLvoid*)&_vertices[0].textureCoordinate);
		assert(glGetError() == GL_NO_ERROR);
	}
	else if (_isC3b)
	{
		// TODO:obj���邢��c3t/c3b�Ń��b�V���f�[�^�͈�ł���O��
		C3bLoader::MeshData* meshData = _meshDatas->meshDatas[0];
		for (int i = 0, offset = 0; i < meshData->numAttribute; ++i)
		{
			const C3bLoader::MeshVertexAttribute& attrib = meshData->attributes[i];
			if (!_isCpuMode)
			{
				glVertexAttribPointer((GLuint)attrib.location, attrib.size, attrib.type, GL_FALSE, _perVertexByteSize, (GLvoid*)&meshData->vertices[offset]);
				assert(glGetError() == GL_NO_ERROR);
			}
			offset += attrib.size;
		}
	}

	glBindTexture(GL_TEXTURE_2D, _texture->getTextureId());
	assert(glGetError() == GL_NO_ERROR);
	glDrawElements(GL_TRIANGLES, _indices.size(), GL_UNSIGNED_SHORT, &_indices[0]);
	assert(glGetError() == GL_NO_ERROR);
}

} // namespace mgrrenderer
