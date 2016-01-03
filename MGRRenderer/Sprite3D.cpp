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
			Logger::log(err.c_str());
			return false;
		}

		_vertices.clear();

		// materialListは現状無視
		// TODO:そもそもMeshDataはその時点でマテリアルごとにまとまってないのでは？faceGroupだからまとまってる？
		// →まとまってる。しかし、今回はマテリアルは一種類という前提でいこう
		// 本来は、std::vector<std::vector<Position3DTextureCoordinates>> がメンバ変数になってて、マテリアルごとに切り替えて描画する
		// テクスチャも本来は切り替え前提だからsetTextureってメソッドおかしいよな。。
		assert(meshList.size() == 1);
		const ObjLoader::MeshData& mesh = meshList[0];
		_vertices = mesh.vertices;
		_indices = mesh.indices;
	}
	else if (ext == ".c3t" || ext == ".c3b")
	{
		_isC3b = true; // TODO:このフラグ分けは非常にださい

		_meshDatas = new (std::nothrow)C3bLoader::MeshDatas();
		C3bLoader::MaterialDatas* materialDatas = new (std::nothrow)C3bLoader::MaterialDatas();
		_nodeDatas = new (std::nothrow)C3bLoader::NodeDatas();
		_animationDatas = new (std::nothrow)C3bLoader::AnimationDatas();
		std::string err;
		if (ext == ".c3t")
		{
			err = C3bLoader::loadC3t(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}
		else
		{
			assert(ext == ".c3b");
			err = C3bLoader::loadC3b(filePath, *_meshDatas, *materialDatas, *_nodeDatas, *_animationDatas);
		}

		if (!err.empty())
		{
			Logger::log(err.c_str());
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
		// TODO:まだc3bには未対応
		assert(false);
	}

	if (_isObj)
	{
		_glData = createOpenGLProgram(
			// vertex shader
			// ModelDataしか使わない場合
			"attribute vec4 a_position;"
			"attribute vec2 a_texCoord;"
			"varying vec2 v_texCoord;"
			"uniform mat4 u_modelMatrix;"
			"uniform mat4 u_viewMatrix;"
			"uniform mat4 u_projectionMatrix;"
			"void main()"
			"{"
			"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
			"	v_texCoord = a_texCoord;"
			"}"
			,
			// fragment shader
			"uniform sampler2D u_texture;"
			"varying vec2 v_texCoord;"
			"void main()"
			"{"
			"	gl_FragColor = texture2D(u_texture, v_texCoord);" // テクスチャ番号は0のみに対応
			"}"
			);
	}
	else if (_isC3b)
	{
		if (_isCpuMode)
		{
			_glData = createOpenGLProgram(
				// vertex shader
				// ModelDataしか使わない場合
				"attribute vec4 a_position;"
				"attribute vec2 a_texCoord;"
				"varying vec2 v_texCoord;"
				"uniform mat4 u_modelMatrix;"
				"uniform mat4 u_viewMatrix;"
				"uniform mat4 u_projectionMatrix;"
				"void main()"
				"{"
				"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * a_position;"
				"	v_texCoord = a_texCoord;"
				"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3bの事情によるもの
				"}"
				,
				// fragment shader
				"uniform sampler2D u_texture;"
				"varying vec2 v_texCoord;"
				"void main()"
				"{"
				"	gl_FragColor = texture2D(u_texture, v_texCoord);" // テクスチャ番号は0のみに対応
				"}"
				);
		}
		else
		{
			_glData = createOpenGLProgram(
				// vertex shader
				// ModelDataしか使わない場合
				//"attribute vec4 a_position;"
				//"attribute vec2 a_texCoord;"
				//"varying vec2 v_texCoord;"
				//"uniform mat4 u_modelMatrix;"
				//"uniform mat4 u_viewMatrix;"
				//"uniform mat4 u_projectionMatrix;"
				//"void main()"
				//"{"
				//"	gl_Position = u_projectionMatrix * u_viewMatrix * a_position;"
				//"	v_texCoord = a_texCoord;"
				//"	v_texCoord.y = 1.0 - v_texCoord.y;"
				//"}"
				//// アニメーションを使う場合
				"attribute vec3 a_position;" // これがvec3になっているのに注意 TODO:なぜなのか？
				"attribute vec2 a_texCoord;"
				"attribute vec4 a_blendWeight;"
				"attribute vec4 a_blendIndex;"
				""
				"const int SKINNING_JOINT_COUNT = 60;" // TODO:なぜ60個までなのか？
				""
				"uniform mat4 u_modelMatrix;"
				"uniform mat4 u_viewMatrix;"
				"uniform mat4 u_projectionMatrix;"
				"uniform mat4 u_matrixPalette[SKINNING_JOINT_COUNT];"
				""
				"varying vec2 v_texCoord;"
				""
				"vec4 getPosition()"
				"{"
				"	mat4 skinMatrix = u_matrixPalette[int(a_blendIndex[0])] * a_blendWeight[0];"
				""
				"	if (a_blendWeight[1] > 0.0)"
				"	{"
				"		skinMatrix += u_matrixPalette[int(a_blendIndex[1])] * a_blendWeight[1];"
				""
				"		if (a_blendWeight[2] > 0.0)"
				"		{"
				"			skinMatrix += u_matrixPalette[int(a_blendIndex[2])] * a_blendWeight[2];"
				""
				"			if (a_blendWeight[3] > 0.0)"
				"			{"
				"				skinMatrix += u_matrixPalette[int(a_blendIndex[3])] * a_blendWeight[3];"
				"			}"
				"		}"
				"	}"
				""
				"	vec4 position = vec4(a_position, 1.0);"
				"	vec4 skinnedPosition = skinMatrix * position;"
				"	skinnedPosition.w = 1.0;"
				"	return skinnedPosition;"
				"}"
				""
				"void main()"
				"{"
				"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * getPosition();"
				"	v_texCoord = a_texCoord;"
				"	v_texCoord.y = 1.0 - v_texCoord.y;" // c3bの事情によるもの
				"}"
				//
				// cocosのやつ
				//
				//"attribute vec3 a_position;"
				//"attribute vec4 a_blendWeight;"
				//"attribute vec4 a_blendIndex;"
				//""
				//"attribute vec2 a_texCoord;"
				//""
				//"const int SKINNING_JOINT_COUNT = 60;"
				//""
				//"uniform vec4 u_matrixPalette[SKINNING_JOINT_COUNT * 3];"
				//""
				//"varying vec2 v_texCoord;"
				//"uniform mat4 u_modelMatrix;"
				//"uniform mat4 u_viewMatrix;"
				//"uniform mat4 u_projectionMatrix;"
				//""
				//"vec4 getPosition()"
				//"{"
				//"	float blendWeight = a_blendWeight[0];"
				//""
				//"	int matrixIndex = int(a_blendIndex[0]) * 3;"
				//"	vec4 matrixPalette1 = u_matrixPalette[matrixIndex] * blendWeight;"
				//"	vec4 matrixPalette2 = u_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"	vec4 matrixPalette3 = u_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//""
				//"	blendWeight = a_blendWeight[1];"
				//"	if (blendWeight > 0.0)"
				//"	{"
				//"		matrixIndex = int(a_blendIndex[1]) * 3;"
				//"		matrixPalette1 += u_matrixPalette[matrixIndex] * blendWeight;"
				//"		matrixPalette2 += u_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"		matrixPalette3 += u_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//"		blendWeight = a_blendWeight[2];"
				//"		if (blendWeight > 0.0)"
				//"		{"
				//"			matrixIndex = int(a_blendIndex[2]) * 3;"
				//"			matrixPalette1 += u_matrixPalette[matrixIndex] * blendWeight;"
				//"			matrixPalette2 += u_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"			matrixPalette3 += u_matrixPalette[matrixIndex + 2] * blendWeight;"
				//""
				//"			blendWeight = a_blendWeight[3];"
				//"			if (blendWeight > 0.0)"
				//"			{"
				//"				matrixIndex = int(attr_blendIndex[3]) * 3;"
				//"				matrixPalette1 += u_matrixPalette[matrixIndex] * blendWeight;"
				//"				matrixPalette2 += u_matrixPalette[matrixIndex + 1] * blendWeight;"
				//"				matrixPalette3 += u_matrixPalette[matrixIndex + 2] * blendWeight;"
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
				//"	gl_Position = u_projectionMatrix * u_viewMatrix * u_modelMatrix * position;"
				//""
				//"	v_texCoord = attr_texCoord;"
				//"	v_texCoord.y = 1.0 - v_texCoord.y;"
				//"}"
				,
				// fragment shader
				"uniform sampler2D u_texture;"
				"varying vec2 v_texCoord;"
				"void main()"
				"{"
				"	gl_FragColor = texture2D(u_texture, v_texCoord);" // テクスチャ番号は0のみに対応
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

	_glData.uniformTexture = glGetUniformLocation(_glData.shaderProgram, "u_texture");
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
		_glData.uniformSkinMatrixPalette = glGetUniformLocation(_glData.shaderProgram, "u_matrixPalette");
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
	// Textureをロードし、pngやjpegを生データにし、OpenGLにあげる仕組みを作らねば。。Spriteのソースを見直すときだ。
	Image image; // ImageはCPU側のメモリを使っているのでこのスコープで解放されてもよいものだからスタックに取る
	image.initWithFilePath(filePath);

	_texture = new Texture(); // TextureはGPU側のメモリを使ってるので解放されると困るのでヒープにとる
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
			// 見つかったらそれを返す。ループも終了。
			return child;
		}
		else if (child->children.size() > 0)
		{
			// 見つからず、子がいるなら子を探しに行く
			C3bLoader::NodeData* findResult = findJointByName(jointName, child->children);
			if (findResult != nullptr)
			{
				// 見つかったらそれを返す。ループも終了。
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

	float t = 0; // 0<=t<=1のアニメーション補完パラメータ
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

	// アニメーションを行う。Animate3D::updateを参考に C3bLoader::AnimationDataの使いこなしが肝だ
	size_t numSkinJoint = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones.size();
	assert(numSkinJoint == _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose.size());

	// 先に各ジョイントのアニメーション行列を作成する
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		// キーフレーム情報がなかったら、NodeDatas::skeletons::transformの方のツリーから再帰的にジョイント位置座標変換配列を計算する
		// まずは対応するボーンを探し、設定されている行列を取得する
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			Logger::log("nodesの方にあったものでskeletonsからボーン名で検索したがない。");
		}
		assert(node != nullptr);

		// 一旦、アニメーション使わない方に初期化
		node->animatedTransform = Mat4::ZERO;

		if (_currentAnimation != nullptr && _currentAnimation->translationKeyFrames.find(jointName) != _currentAnimation->translationKeyFrames.end())
		{
			// アニメーション中で、データにそのボーンのキーフレーム情報があったときはそちらから作った行列で上書きする
			const Vec3& translation = _currentAnimation->evaluateTranslation(jointName, t);
			const Quaternion& rotation = _currentAnimation->evaluateRotation(jointName, t);
			const Vec3& scale = _currentAnimation->evaluateScale(jointName, t);

			node->animatedTransform = Mat4::createTransform(translation, rotation, scale);
		}
	}

	// 次にジョイントのマトリックスパレットをすべて求める
	for (int i = 0; i < numSkinJoint; ++i)
	{
		const std::string& jointName = _nodeDatas->nodes[0]->modelNodeDatas[0]->bones[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		// キーフレーム情報がなかったら、NodeDatas::skeletons::transformの方のツリーから再帰的にジョイント位置座標変換配列を計算する
		// まずは対応するボーンを探し、設定されている行列を取得する
		C3bLoader::NodeData* node = findJointByName(jointName, _nodeDatas->skeleton);
		if (node == nullptr)
		{
			Logger::log("nodesの方にあったものでskeletonsからボーン名で検索したがない。");
		}
		assert(node != nullptr);

		Mat4 transform = (node->animatedTransform != Mat4::ZERO) ? node->animatedTransform : node->transform;

		// 親をルートまでさかのぼってワールド行列を計算する
		while (node->parent != nullptr)
		{
			// TODO:親のボーンもアニメーションしてる可能性あったために、NodeDataにもう一個Matrix持たせるという強引な方法。本来はSkeleton3DとBone3Dみたいに別の階層構造データを保持した方がいい
			const Mat4& parentTransform = (node->parent->animatedTransform != Mat4::ZERO) ? node->parent->animatedTransform : node->parent->transform;
			transform = parentTransform * transform;
			node = node->parent;
		}

		// ボーンの配置行列
		const Mat4& invBindPose = _nodeDatas->nodes[0]->modelNodeDatas[0]->invBindPose[i];//TODO: nodes下に要素は一個、parts下にも一個だけであることを前提にしている

		Mat4 matrix = transform * invBindPose;
		_matrixPalette.push_back(matrix);
		//_matrixPalette.push_back(Vec4(matrix.m[0][0],matrix.m[1][0],matrix.m[2][0],matrix.m[3][0]));
		//_matrixPalette.push_back(Vec4(matrix.m[0][1],matrix.m[1][1],matrix.m[2][1],matrix.m[3][1]));
		//_matrixPalette.push_back(Vec4(matrix.m[0][2],matrix.m[1][2],matrix.m[2][2],matrix.m[3][2]));
	}
}

void Sprite3D::render()
{
	// cocos2d-xはTriangleCommand発行してる形だからな。。テクスチャバインドはTexture2Dでやってるのに大丈夫か？
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
		// TODO:objあるいはc3t/c3bでメッシュデータは一個である前提
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
