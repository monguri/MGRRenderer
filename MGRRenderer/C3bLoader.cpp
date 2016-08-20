#include "C3bLoader.h"
#include "FileUtility.h"
#include "external/json/document.h"
#include "BinaryReader.h"
#if defined(MGRRENDERER_USE_DIRECT3D)
#include "D3DProgram.h"
#endif

namespace mgrrenderer
{

namespace C3bLoader
{
	enum class SeekDataType : unsigned int
	{
		SCENE = 1,
		NODE = 2,
		ANIMATIONS = 3,
		ANIMATION = 4,
		ANIMATION_CHANNEL = 5,
		MODEL = 10,
		MATERIAL = 16,
		EFFECT = 18,
		CAMERA = 32,
		LIGHT = 33,
		MESH = 34,
		MESH_PART = 35,
		MESH_SKIN = 36,
	};

	struct SeekData
	{
		std::string id;
		SeekDataType type;
		unsigned int offset;
	};

	static bool seek(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, SeekDataType type, const std::string& id = "")
	{
		for (const SeekData& data : seekPointTable)
		{
			// id指定がある場合はid一致を確認する
			if (id != "" && id != data.id)
			{
				continue;
			}

			if (type != data.type)
			{
				continue;
			}

			binary.seek(data.offset, SEEK_SET);
			return true;
		}

		return false;
	}

#if defined(MGRRENDERER_USE_DIRECT3D)
	static std::string parseD3DProgramAttributeString(const std::string& str)
	{
		if (str == "VERTEX_ATTRIB_POSITION")
		{
			return D3DProgram::SEMANTIC_POSITION;
		}
		else if (str == "VERTEX_ATTRIB_COLOR")
		{
			return D3DProgram::SEMANTIC_COLOR;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD")
		{
			return D3DProgram::SEMANTIC_TEXTURE_COORDINATE;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD1")
		{
			return D3DProgram::SEMANTIC_TEXTURE_COORDINATE_1;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD2")
		{
			return D3DProgram::SEMANTIC_TEXTURE_COORDINATE_2;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD3")
		{
			return D3DProgram::SEMANTIC_TEXTURE_COORDINATE_3;
		}
		else if (str == "VERTEX_ATTRIB_NORMAL")
		{
			return D3DProgram::SEMANTIC_NORMAL;
		}
		else if (str == "VERTEX_ATTRIB_BLEND_WEIGHT")
		{
			return D3DProgram::SEMANTIC_BLEND_WEIGHT;
		} 
		else if (str == "VERTEX_ATTRIB_BLEND_INDEX")
		{
			return D3DProgram::SEMANTIC_BLEND_INDEX;
		}
		else
		{
			Logger::logAssert(false, "Wrong Attribute type.");
			return "";
		}
	}
#elif defined(MGRRENDERER_USE_OPENGL)
	static GLenum parseGLTypeString(const std::string& str)
	{
		if (str == "GL_BYTE")
		{
			return GL_BYTE;
		}
		else if (str == "GL_UNSIGNED_BYTE")
		{
			return GL_UNSIGNED_BYTE;
		}
		else if (str == "GL_SHORT")
		{
			return GL_SHORT;
		}
		else if (str == "GL_UNSIGNED_SHORT")
		{
			return GL_UNSIGNED_SHORT;
		}
		else if (str == "GL_INT")
		{
			return GL_INT;
		}
		else if (str == "GL_UNSIGNED_INT")
		{
			return GL_UNSIGNED_INT;
		}
		else if (str == "GL_FLOAT")
		{
			return GL_FLOAT;
		}
		else if (str == "REPEAT")
		{
			return GL_REPEAT;
		}
		else if (str == "CLAMP")
		{
			return GL_CLAMP_TO_EDGE;
		}
		else
		{
			Logger::logAssert(false, "Invalid GL str.");
			return 0;
		}
	}

	static GLProgram::AttributeLocation parseGLProgramAttributeString(const std::string& str)
	{
		if (str == "VERTEX_ATTRIB_POSITION")
		{
			return GLProgram::AttributeLocation::POSITION;
		}
		else if (str == "VERTEX_ATTRIB_COLOR")
		{
			return GLProgram::AttributeLocation::COLOR;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD")
		{
			return GLProgram::AttributeLocation::TEXTURE_COORDINATE;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD1")
		{
			return GLProgram::AttributeLocation::TEXTURE_COORDINATE_1;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD2")
		{
			return GLProgram::AttributeLocation::TEXTURE_COORDINATE_2;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD3")
		{
			return GLProgram::AttributeLocation::TEXTURE_COORDINATE_3;
		}
		else if (str == "VERTEX_ATTRIB_NORMAL")
		{
			return GLProgram::AttributeLocation::NORMAL;
		}
		else if (str == "VERTEX_ATTRIB_BLEND_WEIGHT")
		{
			return GLProgram::AttributeLocation::BLEND_WEIGHT;
		} 
		else if (str == "VERTEX_ATTRIB_BLEND_INDEX")
		{
			return GLProgram::AttributeLocation::BLEND_INDEX;
		}
		else
		{
			Logger::logAssert(false, "Wrong Attribute type.");
			return GLProgram::AttributeLocation::NONE;
		}
	}
#endif // MGRRENDERER_USE_OPENGL
	
	static TextureData::Usage parseTextureTypeString(const std::string& str)
	{
		if (str == "AMBIENT")
		{
			return TextureData::Usage::AMBIENT;
		}
		else if (str == "BUMP")
		{
			return TextureData::Usage::BUMP;
		}
		else if (str == "DIFFUSE")
		{
			return TextureData::Usage::DIFFUSE;
		}
		else if (str == "EMISSIVE")
		{
			return TextureData::Usage::EMISSIVE;
		}
		else if (str == "NONE")
		{
			return TextureData::Usage::NONE;
		}
		else if (str == "NORMAL")
		{
			return TextureData::Usage::NORMAL;
		}
		else if (str == "REFLECTION")
		{
			return TextureData::Usage::REFLECTION;
		}
		else if (str == "SHININESS")
		{
			return TextureData::Usage::SHININESS;
		}
		else if (str == "SPECULAR")
		{
			return TextureData::Usage::SPECULAR;
		}
		else if (str == "TRANSPARENCY")
		{
			return TextureData::Usage::TRANSPARENCY;
		}
		else
		{
			Logger::logAssert(false, "Wrong Texture type");
			return TextureData::Usage::UNKNOWN;
		}
	}

	static std::string loadMeshDatasFromJson_0_1(const rapidjson::Document& json, MeshDatas& outMeshDatas)
	{
		const rapidjson::Value& meshesVal = json["meshes"];
		MeshData* mesh = new (std::nothrow)MeshData();
		const rapidjson::Value& meshVal = meshesVal[(rapidjson::SizeType)0];

		// attribute
		const rapidjson::Value& attributesVal = meshVal["attributes"];
		MeshVertexAttribute attrib;
		mesh->numAttribute = attributesVal.Size();
		mesh->attributes.resize(mesh->numAttribute);

		for (rapidjson::SizeType j = 0; j < mesh->numAttribute; ++j)
		{
			const rapidjson::Value& attributeVal = attributesVal[j];

			int size = attributeVal["size"].GetInt();
			const std::string& type = attributeVal["type"].GetString();
			const std::string& attribute = attributeVal["attribute"].GetString();
	
			attrib.size = size;
			attrib.attributeSizeBytes = sizeof(float) * size;
#if defined(MGRRENDERER_USE_DIRECT3D)
			attrib.semantic = parseD3DProgramAttributeString(type);
			(void)attribute; //未使用変数警告抑制
#elif defined(MGRRENDERER_USE_OPENGL)
			attrib.type = parseGLTypeString(type);
			attrib.location = parseGLProgramAttributeString(attribute);
#endif
			mesh->attributes[j] = attrib;
		}
			
		// vertices
		const rapidjson::Value& meshBodyVal0 = meshVal["defaultpart"][(rapidjson::SizeType)0];// キー文字列があってるか確認せよ
		const rapidjson::Value& verticesVal = meshBodyVal0["vertices"];
		mesh->vertexSizeInFloat = meshBodyVal0["vertexsize"].GetInt();
		mesh->vertices.resize(mesh->vertexSizeInFloat); // TODO:微妙に0_1でないものと実装違う
		rapidjson::SizeType verticesValSize = verticesVal.Size();
		for (rapidjson::SizeType j = 0; j < verticesValSize; ++j)
		{
			mesh->vertices.push_back(verticesVal[j].GetDouble());
		}
			
		unsigned int numIndex = meshBodyVal0["indexnum"].GetUint();
		std::vector<unsigned short> indexArray;
		indexArray.resize(numIndex);
			
		const rapidjson::Value& indicesVal = meshBodyVal0["indices"];
		rapidjson::SizeType indicesValSize = indicesVal.Size();
		for (rapidjson::SizeType j = 0; j < indicesValSize; ++j)
		{
			indexArray[j] = (unsigned short)indicesVal[j].GetUint();
		}
		mesh->subMeshIndices.push_back(indexArray);
		//TODO:aabbsについてはとりあえず放置
		
		outMeshDatas.meshDatas.push_back(mesh);
		return "";
	}

	static std::string loadMeshDatasFromBinary_0_1(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, MeshDatas& outMeshDatas)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MESH);
		if (!success)
		{
			return "seek failed, SeekDataType::MESH";
		}

		MeshData* mesh = new (std::nothrow)MeshData();

		//
		// attributes
		//
		unsigned int attribSize = 0;
		size_t readCount = binary.read(&attribSize, 4, 1);
		if (readCount != 1 || attribSize < 1)
		{
			delete mesh;
			return "warning: Failed to read meshdata: attribCount";
		}

		enum class VertexAttribType : unsigned int
		{
			POSITION = 0,
			COLOR = 1,
			TEX_COORD = 2,
			NORMAL = 3,
			BLEND_WEIGHT = 4,
			BLEND_INDEX = 5,
		};

		for (unsigned int i = 0; i < attribSize; ++i)
		{
			VertexAttribType attribType; 
			unsigned int size;
			readCount = binary.read(&attribType, 4, 1);
			if (readCount != 1)
			{
				delete mesh;
				return "warning: Failed to read meshdata : attribType.";
			}

			readCount = binary.read(&size, 4, 1);
			if (readCount != 1)
			{
				delete mesh;
				return "warning: Failed to read meshdata : size.";
			}

			MeshVertexAttribute attrib;
			attrib.size = size;
			attrib.attributeSizeBytes = size * sizeof(float);
#if defined(MGRRENDERER_USE_OPENGL)
			attrib.type = GL_FLOAT;
#endif

			switch (attribType)
			{
			case VertexAttribType::POSITION:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_POSITION;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::POSITION;
#endif
				break;
			case VertexAttribType::COLOR:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_COLOR;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::COLOR;
#endif
				break;
			case VertexAttribType::TEX_COORD:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_TEXTURE_COORDINATE;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::TEXTURE_COORDINATE;
#endif
				break;
			case VertexAttribType::NORMAL:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_NORMAL;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::NORMAL;
#endif
				break;
			case VertexAttribType::BLEND_WEIGHT:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_BLEND_WEIGHT;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::BLEND_WEIGHT;
#endif
				break;
			case VertexAttribType::BLEND_INDEX:
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = D3DProgram::SEMANTIC_BLEND_INDEX;
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.location = GLProgram::AttributeLocation::BLEND_INDEX;
#endif
				break;
			default:
				Logger::logAssert(false, "想定していないアトリビュート変数タイプ");
				break;
			}

			mesh->attributes.push_back(attrib);
		}

		// 
		// vertices
		//
		readCount = binary.read(&mesh->vertexSizeInFloat, 4, 1);
		if (readCount != 1 || mesh->vertexSizeInFloat == 0)
		{
			delete mesh;
			return "warning: Failed to read meshdata: vertexSizeInFloat";
		}

		mesh->vertices.resize(mesh->vertexSizeInFloat);
		readCount = binary.read(mesh->vertices.data(), 4, mesh->vertexSizeInFloat);
		if (readCount != mesh->vertexSizeInFloat)
		{
			delete mesh;
			return "warning: Failed to read meshdata: vertex element";
		}

		//
		// indices
		//
		unsigned int meshPartCount = 1;
		for (unsigned int i = 0; i < meshPartCount; ++i)
		{
			unsigned int indexCount;
			readCount = binary.read(&indexCount, 4, 1);
			if (readCount != 1)
			{
				delete mesh;
				return "warning: Failed to read meshdata: indexCount";
			}

			std::vector<unsigned short> indices;
			indices.resize(indexCount);
			readCount = binary.read(indices.data(), 2, indexCount);
			if (readCount != indexCount)
			{
				delete mesh;
				return "warning: Failed to read meshdata: indices";
			}

			mesh->subMeshIndices.push_back(indices);
		}

		outMeshDatas.meshDatas.push_back(mesh);
		return "";
	}

	static std::string loadMeshDatasFromJson(const rapidjson::Document& json, MeshDatas& outMeshDatas)
	{
		const rapidjson::Value& meshesVal = json["meshes"];
		rapidjson::SizeType meshesValSize = meshesVal.Size();
		for (rapidjson::SizeType i = 0; i < meshesValSize; ++i)
		{
			MeshData* mesh = new (std::nothrow)MeshData();
			const rapidjson::Value& meshVal = meshesVal[i];

			// attribute
			const rapidjson::Value& attributesVal = meshVal["attributes"];
			MeshVertexAttribute attrib;
			mesh->numAttribute = attributesVal.Size();
			mesh->attributes.resize(mesh->numAttribute);

			for (rapidjson::SizeType j = 0; j < mesh->numAttribute; ++j)
			{
				const rapidjson::Value& attributeVal = attributesVal[j];

				int size = attributeVal["size"].GetInt();
				const std::string& type = attributeVal["type"].GetString();
				const std::string& attribute = attributeVal["attribute"].GetString();

				attrib.size = size;
				attrib.attributeSizeBytes = sizeof(float) * size;
#if defined(MGRRENDERER_USE_DIRECT3D)
				attrib.semantic = parseD3DProgramAttributeString(attribute);
				(void)type; // 未使用変数の警告抑制
#elif defined(MGRRENDERER_USE_OPENGL)
				attrib.type = parseGLTypeString(type);
				attrib.location = parseGLProgramAttributeString(attribute);
#endif
				mesh->attributes[j] = attrib;
			}
			
			// vertices
			const rapidjson::Value& verticesVal = meshVal["vertices"];
			mesh->vertexSizeInFloat = verticesVal.Size();
			for (rapidjson::SizeType j = 0; j < mesh->vertexSizeInFloat; ++j)
			{
				mesh->vertices.push_back(verticesVal[j].GetDouble());
			}
			
			// mesh part
			const rapidjson::Value& partsVal = meshVal["parts"];
			size_t numParts = partsVal.Size();
			for (rapidjson::SizeType j = 0; j < numParts; ++j)
			{
				std::vector<unsigned short> indexArray;
				const rapidjson::Value& partVal = partsVal[j];
				mesh->subMeshIds.push_back(partVal["id"].GetString());
				
				const rapidjson::Value& indicesVal = partVal["indices"];
				size_t numIndex = indicesVal.Size();
				for (rapidjson::SizeType k = 0; k < numIndex; ++k)
				{
					indexArray.push_back(indicesVal[k].GetUint());
				}
				
				mesh->subMeshIndices.push_back(indexArray);
				mesh->numSubMesh = mesh->subMeshIndices.size();
			}
			
			//TODO:aabbsについてはとりあえず放置
		
			// TODO:push_backはコスト高いがとりあえず
			outMeshDatas.meshDatas.push_back(mesh);
		}
		return "";
	}

	static std::string loadMeshDatasFromBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, MeshDatas& outMeshDatas)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MESH);
		if (!success)
		{
			return "seek failed, SeekDataType::MESH";
		}

		unsigned int meshSize;
		size_t readCount = binary.read(&meshSize, 4, 1);
		if (readCount != 1 || meshSize < 1)
		{
			return "warning: Failed to read meshdata: attribCount";
		}

		for (unsigned int i = 0; i < meshSize; ++i)
		{
			MeshData* mesh = new (std::nothrow)MeshData();

			//
			// attributes
			//
			unsigned int attribSize = 0;
			readCount = binary.read(&attribSize, 4, 1);
			if (readCount != 1 || attribSize < 1)
			{
				delete mesh;
				outMeshDatas.resetData();
				return "warning: Failed to read meshdata: attribCount";
			}

			mesh->numAttribute = attribSize;
			mesh->attributes.resize(attribSize);

			for (unsigned int j = 0; j < attribSize; ++j)
			{
				unsigned int size;
				readCount = binary.read(&size, 4, 1);
				if (readCount != 1)
				{
					delete mesh;
					outMeshDatas.resetData();
					return "warning: Failed to read meshdata : size.";
				}

				const std::string& type = binary.readString();
				const std::string& attribStr = binary.readString();
				mesh->attributes[j].size = size;
				mesh->attributes[j].attributeSizeBytes = size * sizeof(float);
#if defined(MGRRENDERER_USE_DIRECT3D)
				mesh->attributes[j].semantic = parseD3DProgramAttributeString(attribStr);
				(void)type; // 未使用変数の警告抑制
#elif defined(MGRRENDERER_USE_OPENGL)
				mesh->attributes[j].type = parseGLTypeString(type);
				mesh->attributes[j].location = parseGLProgramAttributeString(attribStr);
#endif
			}

			// 
			// vertices
			//
			readCount = binary.read(&mesh->vertexSizeInFloat, 4, 1);
			if (readCount != 1 || mesh->vertexSizeInFloat == 0)
			{
				delete mesh;
				outMeshDatas.resetData();
				return "warning: Failed to read meshdata: vertexSizeInFloat";
			}

			mesh->vertices.resize(mesh->vertexSizeInFloat);
			readCount = binary.read(mesh->vertices.data(), 4, mesh->vertexSizeInFloat);
			if (readCount != mesh->vertexSizeInFloat)
			{
				delete mesh;
				outMeshDatas.resetData();
				return "warning: Failed to read meshdata: vertex element";
			}

			//
			// indices
			//
			unsigned int meshPartCount = 1;
			readCount = binary.read(&meshPartCount, 4, 1);
			if (readCount != 1)
			{
				delete mesh;
				outMeshDatas.resetData();
				return "warning: Failed to read meshdata: mesh part count";
			}

			for (unsigned int j = 0; j < meshPartCount; ++j)
			{
				const std::string& meshPartId = binary.readString();
				mesh->subMeshIds.push_back(meshPartId);

				unsigned int indexCount;
				readCount = binary.read(&indexCount, 4, 1);
				if (readCount != 1)
				{
					delete mesh;
					outMeshDatas.resetData();
					return "warning: Failed to read meshdata: indexCount";
				}

				std::vector<unsigned short> indices;
				indices.resize(indexCount);
				readCount = binary.read(indices.data(), 2, indexCount);
				if (readCount != indexCount)
				{
					delete mesh;
					outMeshDatas.resetData();
					return "warning: Failed to read meshdata: indices";
				}

				mesh->subMeshIndices.push_back(indices);
				mesh->numSubMesh = indexCount;

				// subMeshAABBは省略
			}

			outMeshDatas.meshDatas.push_back(mesh);
		}

		return "";
	}

	static std::string loadMaterialDatasFromJson(const rapidjson::Document& json, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		if (!json.HasMember("materials"))
		{
			return "no materials in json";
		}
		
		const rapidjson::Value& materialsVal = json["materials"];
		rapidjson::SizeType materialsValSize = materialsVal.Size();
		
		for (rapidjson::SizeType i = 0; i < materialsValSize; ++i)
		{
			MaterialData* material = new (std::nothrow)MaterialData();
			const rapidjson::Value& materialVal = materialsVal[i];
			material->id = materialVal["id"].GetString();
			// ambientとかdiffuseとかの値は全無視。まあcocosでも使ってないしね
			
			if (!materialVal.HasMember("textures"))
			{
				continue;
			}
			
			const rapidjson::Value& texturesVal = materialVal["textures"];
			rapidjson::SizeType texturesValSize = texturesVal.Size();
			for (rapidjson::SizeType j = 0; j < texturesValSize; ++j)
			{
				TextureData texture;
				const rapidjson::Value& textureVal = texturesVal[j];
				const std::string& fileName = textureVal["filename"].GetString();
				texture.fileName = fileName.empty() ? "" : dirPath + fileName;
				texture.type = parseTextureTypeString(textureVal["type"].GetString());
#if defined(MGRRENDERER_USE_OPENGL)
				texture.wrapS = parseGLTypeString(textureVal["wrapModeU"].GetString());
				texture.wrapT = parseGLTypeString(textureVal["wrapModeV"].GetString());
#endif
				material->textures.push_back(texture);
			}
			
			const rapidjson::Value& ambientVal = materialVal["ambient"];
			Logger::logAssert(ambientVal.Size() == 3, "ambientのfloatが3つでない");
			material->ambient.color.r = ambientVal[0].GetDouble();
			material->ambient.color.g = ambientVal[1].GetDouble();
			material->ambient.color.b = ambientVal[2].GetDouble();

			const rapidjson::Value& diffuseVal = materialVal["diffuse"];
			Logger::logAssert(diffuseVal.Size() == 3, "diffuseのfloatが3つでない");
			material->diffuse.color.r = diffuseVal[0].GetDouble();
			material->diffuse.color.g = diffuseVal[1].GetDouble();
			material->diffuse.color.b = diffuseVal[2].GetDouble();

			const rapidjson::Value& specularVal = materialVal["specular"];
			Logger::logAssert(specularVal.Size() == 3, "specularのfloatが3つでない");
			material->specular.color.r = specularVal[0].GetDouble();
			material->specular.color.g = specularVal[1].GetDouble();
			material->specular.color.b = specularVal[2].GetDouble();

			const rapidjson::Value& emissiveVal = materialVal["emissive"];
			Logger::logAssert(emissiveVal.Size() == 3, "emissiveのfloatが3つでない");
			material->emissive.color.r = emissiveVal[0].GetDouble();
			material->emissive.color.g = emissiveVal[1].GetDouble();
			material->emissive.color.b = emissiveVal[2].GetDouble();

			material->opacity = materialVal["opacity"].GetDouble();
			material->shininess = materialVal["shininess"].GetDouble();

			outMaterialDatas.materialDatas.push_back(material);
		}
		
		return "";
	}

	static std::string loadMaterialDatasFromBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MATERIAL);
		if (!success)
		{
			return "seek failed, SeekDataType::MATERIAL";
		}

		unsigned int numMaterial;
		size_t readCount = binary.read(&numMaterial, 4, 1);
		if (readCount != 1)
		{
			return "warning: Failed to read Materialdata: num material";
		}

		for (unsigned int i = 0; i < numMaterial; ++i)
		{
			MaterialData* material = new (std::nothrow)MaterialData();
			material->id = binary.readString();

			// diffuse(3), ambient(3), emissive(3), opacity(1), specular(3), shininess(1)
			float parameter[14];
			readCount = binary.read(parameter, 4, 14);
			if (readCount != 14)
			{
				delete material;
				outMaterialDatas.resetData();
				return "warning: Failed to read Materialdata: color";
			}

			material->diffuse = Color3F(parameter[0], parameter[1], parameter[2]);
			material->ambient = Color3F(parameter[3], parameter[4], parameter[5]);
			material->emissive = Color3F(parameter[6], parameter[7], parameter[8]);
			material->opacity = parameter[9];
			material->specular = Color3F(parameter[10], parameter[11], parameter[12]);
			material->shininess = parameter[13];

			unsigned int numTexture;
			readCount = binary.read(&numTexture, 4, 1);
			if (readCount != 1)
			{
				delete material;
				outMaterialDatas.resetData();
				return "warning: Failed to read Materialdata: num texture";
			}

			for (unsigned int j = 0; j < numTexture; ++j)
			{
				TextureData texture;
				texture.id = binary.readString();
				if (texture.id.empty())
				{
					delete material;
					outMaterialDatas.resetData();
					return "warning: Failed to read Materialdata: texturePath is empty";
				}

				const std::string& texturePath = binary.readString();
				if (texturePath.empty())
				{
					delete material;
					outMaterialDatas.resetData();
					return "warning: Failed to read Materialdata: texturePath is empty";
				}

				texture.fileName = dirPath + texturePath;

				// 使ってない
				float uv[4];
				binary.read(&uv, 4, 4);

				texture.type = parseTextureTypeString(binary.readString());
#if defined(MGRRENDERER_USE_OPENGL)
				texture.wrapS = parseGLTypeString(binary.readString());
				texture.wrapT = parseGLTypeString(binary.readString());
#endif
				material->textures.push_back(texture);
			}

			outMaterialDatas.materialDatas.push_back(material);
		}
		
		return "";
	}

	static std::string loadMaterialDatasFromJson_0_1(const rapidjson::Document& json, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		if (!json.HasMember("material"))
		{
			return "no material in json";
		}
		
		const rapidjson::Value& materialsVal = json["material"];
		if (materialsVal.Size() == 0)
		{
			return "no material body";
		}
		
		MaterialData* material = new (std::nothrow)MaterialData();
		const rapidjson::Value& materialVal0 = materialsVal[(rapidjson::SizeType)0];
		if (!materialVal0.HasMember("base"))
		{
			return "no material base";
		}
			
		const rapidjson::Value& materialBaseVal0 = materialVal0["base"][(rapidjson::SizeType)0];
		
		TextureData texture;
		const std::string& fileName = materialBaseVal0["filename"].GetString();
		texture.fileName = fileName.empty() ? "" : dirPath + fileName;
		texture.type = TextureData::Usage::DIFFUSE;
		texture.id = "";
		material->textures.push_back(texture);
		
		outMaterialDatas.materialDatas.push_back(material);
		return "";
	}

	static std::string loadMaterialDatasFromBinary_0_1(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MATERIAL);
		if (!success)
		{
			return "seek failed, SeekDataType::MATERIAL";
		}

		MaterialData* material = new (std::nothrow)MaterialData();

		const std::string& texturePath = binary.readString();
		if (texturePath.empty())
		{
			delete material;
			outMaterialDatas.resetData();
			return "warning: Failed to read Materialdata: texturePath is empty";
		}

		TextureData texture;
		texture.fileName = dirPath + texturePath;
		texture.id = "";
		texture.type = TextureData::Usage::DIFFUSE;
		material->textures.push_back(texture);
		outMaterialDatas.materialDatas.push_back(material);
		return "";
	}

	static std::string loadMaterialDatasFromJson_0_2(const rapidjson::Document& json, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		if (!json.HasMember("material"))
		{
			return "no material in json";
		}
		
		const rapidjson::Value& materialsVal = json["material"];
		
		MaterialData* material = new (std::nothrow)MaterialData();

		rapidjson::SizeType materialsValSize = materialsVal.Size();
		for (rapidjson::SizeType i = 0; i < materialsValSize; ++i)
		{
			const rapidjson::Value& materialVal = materialsVal[i];
			
			TextureData texture;
			const std::string& fileName = materialVal["textures"].GetString();
			texture.fileName = fileName.empty() ? "" : dirPath + fileName;
			texture.type = TextureData::Usage::DIFFUSE;
			texture.id = "";;
			material->textures.push_back(texture);
		}
		outMaterialDatas.materialDatas.push_back(material);
		
		return "";
	}

	static std::string loadMaterialDatasFromBinary_0_2(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, const std::string& dirPath, MaterialDatas& outMaterialDatas)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MATERIAL);
		if (!success)
		{
			return "seek failed, SeekDataType::MATERIAL";
		}

		unsigned int numMaterial;
		size_t readCount = binary.read(&numMaterial, 4, 1);
		if (readCount != 1)
		{
			return "warning: Failed to read Materialdata: num material";
		}

		for (unsigned int i = 0; i < numMaterial; ++i)
		{
			MaterialData* material = new (std::nothrow)MaterialData();

			const std::string& texturePath = binary.readString();
			if (texturePath.empty())
			{
				delete material;
				outMaterialDatas.resetData();
				return "warning: Failed to read Materialdata: texturePath is empty";
			}

			TextureData texture;
			texture.fileName = dirPath + texturePath;
			texture.id = "";
			texture.type = TextureData::Usage::DIFFUSE;
			material->textures.push_back(texture);
			outMaterialDatas.materialDatas.push_back(material);
		}
		
		return "";
	}

	static void getChildMap(std::map<int, std::vector<int>>& outChildMap, SkinData& outSkinData, const rapidjson::Value& val)
	{
		// get transform matrix
		Mat4 transform;
		const rapidjson::Value& parentTransform = val["tansform"];
		rapidjson::SizeType parentTransformSize = parentTransform.Size();
		for (rapidjson::SizeType i = 0; i < parentTransformSize; ++i)
		{
			transform.m[i / 4][i % 4] = parentTransform[i].GetDouble();
		}

		// set origin matrices
		const std::string& parentName = val["id"].GetString();
		int parentNameIndex = outSkinData.getSkinBoneNameIndex(parentName);
		if (parentNameIndex < 0)
		{
			outSkinData.addNodeBoneName(parentName);
			outSkinData.nodeBoneOriginMatrices.push_back(transform);
			parentNameIndex = outSkinData.getBoneNameIndex(parentName);
		}
		else if (parentNameIndex < static_cast<int>(outSkinData.skinBoneNames.size()))
		{
			outSkinData.skinBoneOriginMatrices.push_back(transform);
		}

		// set root bone index
		if (outSkinData.rootBoneIndex < 0)
		{
			outSkinData.rootBoneIndex = parentNameIndex;
		}

		if (!val.HasMember("children"))
		{
			return;
		}

		const rapidjson::Value& childrenVal = val["children"];
		rapidjson::SizeType childrenValSize = childrenVal.Size();
		for (rapidjson::SizeType i = 0; i < childrenValSize; ++i)
		{
			// get child bone name
			const rapidjson::Value& childVal = childrenVal[i];

			const std::string& childName = childVal["id"].GetString();
			int childNameIndex = outSkinData.getSkinBoneNameIndex(childName);
			if (childNameIndex < 0)
			{
				outSkinData.addNodeBoneName(childName);
				childNameIndex = outSkinData.getBoneNameIndex(childName);
			}

			outChildMap[parentNameIndex].push_back(childNameIndex);

			// ツリー構造の再帰
			getChildMap(outChildMap, outSkinData, childVal);
		}
	}

	static bool loadSkinDataFromJson(const rapidjson::Document& json, SkinData& outSkinData)
	{
		if (!json.HasMember("skin"))
		{
			return false;
		}

		const rapidjson::Value& skinDataArrayVal = json["skin"];
		Logger::logAssert(skinDataArrayVal.IsArray(), "c3tのフォーマットがおかしい");

		const rapidjson::Value& skinDataVal0 = skinDataArrayVal[(rapidjson::SizeType)0];

		if (!skinDataVal0.HasMember("bones"))
		{
			return false;
		}

		const rapidjson::Value& skinDataBonesVal = skinDataVal0["bones"];
		rapidjson::SizeType skinDataBonesValSize = skinDataBonesVal.Size();
		for (rapidjson::SizeType i = 0; i < skinDataBonesValSize; ++i)
		{
			const rapidjson::Value& skinDataBone = skinDataBonesVal[i];
			const std::string& name = skinDataBone["node"].GetString();
			outSkinData.addSkinBoneName(name);

			Mat4 matBindPos;
			const rapidjson::Value& bindPos = skinDataBone["bindshape"];
			rapidjson::SizeType bindPosSize = bindPos.Size();
			for (rapidjson::SizeType j = 0; j < bindPosSize; ++j)
			{
				matBindPos.m[j / 4][j % 4] = bindPos[j].GetDouble();
			}
			outSkinData.inverseBindPoseMatrices.push_back(matBindPos);
		}

		// set root bone infomation
		const rapidjson::Value& skinDataVal1 = skinDataArrayVal[(rapidjson::SizeType)1];

		// parent and child relationship map
		outSkinData.skinBoneOriginMatrices.resize(outSkinData.skinBoneNames.size());
		getChildMap(outSkinData.boneChild, outSkinData, skinDataVal1);
		return true;
	}

	static bool loadSkinDataFromBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, SkinData& outSkinData)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::MESH_SKIN);
		if (!success)
		{
			Logger::log("seek failed, SeekDataType::MESH_SKIN");
			return false;
		}

		// transform
		float bindShape[4][4];
		success = binary.readMatrix(&bindShape[0][0]);
		if (!success)
		{
			Logger::log("warning: Failed to read SkinData: bindShape matrix");
			return false;
		}

		// bone count
		unsigned int numBone;
		size_t readCount = binary.read(&numBone, 4, 1);
		if (readCount != 1 || numBone == 0)
		{
			Logger::log("warning: Failed to read SkinData: boneNum");
			return false;
		}

		float bindPos[4][4];
		for (unsigned int i = 0; i < numBone; ++i)
		{
			const std::string& skinBoneName = binary.readString();
			outSkinData.skinBoneNames.push_back(skinBoneName);

			success = binary.readMatrix(&bindPos[0][0]);
			if (!success)
			{
				Logger::log("warning: Failed to load SkinData: bindpos");
				return false;
			}

			Mat4 invBindPose((float**)bindPos);
			outSkinData.inverseBindPoseMatrices.push_back(Mat4((float**)bindPos));
		}

		outSkinData.skinBoneOriginMatrices.resize(numBone);

		const std::string& boneName = binary.readString();
		if (boneName.empty())
		{
			Logger::log("warning: Failed to load SkinData: bone name");
			return false;
		}

		success = binary.readMatrix(&bindShape[0][0]); // なぜか再び読んでいて、前に読んだものは使ってない
		if (!success)
		{
			Logger::log("warning: Failed to load SkinData: bind shape");
			return false;
		}

		int rootIndex = outSkinData.getSkinBoneNameIndex(boneName);
		if (rootIndex < 0)
		{
			outSkinData.addNodeBoneName(boneName);
			rootIndex = outSkinData.getSkinBoneNameIndex(boneName);
			outSkinData.nodeBoneOriginMatrices.push_back(Mat4((float**)bindShape));
		}
		else
		{
			outSkinData.skinBoneOriginMatrices[rootIndex] = Mat4((float**)bindShape);
		}

		outSkinData.rootBoneIndex = rootIndex;

		unsigned int numLink;
		readCount = binary.read(&numLink, 4, 1);
		if (readCount != 1)
		{
			Logger::log("warning: Failed to read SkinData: numLink");
			return false;
		}

		for (unsigned int i = 0; i < numLink; ++i)
		{
			const std::string& id = binary.readString();
			if (id.empty())
			{
				Logger::log("warning: Failed to read SkinData: id");
				return false;
			}

			int index = outSkinData.getSkinBoneNameIndex(id);

			const std::string& parentId = binary.readString();
			if (parentId.empty())
			{
				Logger::log("warning: Failed to read SkinData: parentId");
				return false;
			}

			float transform[4][4];
			success = binary.readMatrix(&transform[0][0]);
			if (!success)
			{
				Logger::log("warning: Failed to load SkinData : transform");
				return false;
			}

			if (index < 0)
			{
				outSkinData.addNodeBoneName(id);
				index = outSkinData.getBoneNameIndex(id);
				outSkinData.nodeBoneOriginMatrices.push_back(Mat4((float**)transform));
			}
			else
			{
				outSkinData.skinBoneOriginMatrices.push_back(Mat4((float**)transform));
			}

			int parentIndex = outSkinData.getSkinBoneNameIndex(parentId);
			if (parentIndex < 0)
			{
				outSkinData.addNodeBoneName(parentId);
				parentIndex = outSkinData.getBoneNameIndex(parentId);
			}

			outSkinData.boneChild[parentIndex].push_back(index);
		}

		return true;
	}

	static NodeData* parseNodeRecursivelyJson(const rapidjson::Value& value, bool isSingleSprite, const std::string& version, NodeData* parent)
	{
		NodeData* nodeData = new (std::nothrow)NodeData();
		// id
		nodeData->id = value["id"].GetString();
		// parent
		nodeData->parent = parent;

		// transform
		Mat4 transform;
		const rapidjson::Value& transformVal = value["transform"];
		rapidjson::SizeType transformValSize = transformVal.Size();

		for (rapidjson::SizeType i = 0; i < transformValSize; ++i)
		{
			transform.m[i / 4][i % 4] = transformVal[i].GetDouble();
		}

		nodeData->transform = transform;

		bool isSkin = false;

		// parts
		if (value.HasMember("parts"))
		{
			const rapidjson::Value& partsVal = value["parts"];
			rapidjson::SizeType partsValSize = partsVal.Size();

			for (rapidjson::SizeType i = 0; i < partsValSize; ++i)
			{
				ModelData* modelData = new (std::nothrow)ModelData();
				const rapidjson::Value& partVal = partsVal[i];
				modelData->subMeshId = partVal["meshpartid"].GetString();
				modelData->materialId = partVal["materialid"].GetString();

				if (modelData->subMeshId.empty() || modelData->materialId.empty())
				{
					Logger::log("warning: Node %s part is missing meshPartId or materialId", nodeData->id.c_str());
					delete modelData;
					delete nodeData;
					return nullptr;
				}

				if (partVal.HasMember("bones"))
				{
					const rapidjson::Value& bonesVal = partVal["bones"];
					rapidjson::SizeType bonesValSize = bonesVal.Size();

					for (rapidjson::SizeType j = 0; j < bonesValSize; ++j)
					{
						const rapidjson::Value& boneVal = bonesVal[j];

						// node
						if (!boneVal.HasMember("node"))
						{
							Logger::log("warning: Bone node ID missing");
							delete modelData;
							delete nodeData;
							return nullptr;
						}

						modelData->bones.push_back(boneVal["node"].GetString());

						Mat4 invBindPos;
						const rapidjson::Value& invBindPosVal = boneVal["transform"];
						rapidjson::SizeType invBindPosValSize = invBindPosVal.Size();
						for (rapidjson::SizeType k = 0; k < invBindPosValSize; ++k)
						{
							invBindPos.m[k / 4][k % 4] = invBindPosVal[k].GetDouble();
						}

						modelData->invBindPose.push_back(invBindPos);
					}

					if (bonesVal.Size() > 0)
					{
						isSkin = true;
					}
				}

				nodeData->modelNodeDatas.push_back(modelData);
			}
		}

		if (version == "0.1" || version == "0.2" || version == "0.3" || version == "0.4" || version == "0.5" || version == "0.6")
		{
			if (isSkin || isSingleSprite)
			{
				nodeData->transform = Mat4::IDENTITY;
			}
			else
			{
				nodeData->transform = transform;
			}
		}
		else
		{
			nodeData->transform = transform;
		}

		if (value.HasMember("children"))
		{
			const rapidjson::Value& childrenVal = value["children"];
			rapidjson::SizeType childrenValSize = childrenVal.Size();

			for (rapidjson::SizeType i = 0; i < childrenValSize; i++)
			{
				const rapidjson::Value& childVal = childrenVal[i];

				NodeData* child = parseNodeRecursivelyJson(childVal, isSingleSprite, version, nodeData);
				// nullptrが返っても加える
				nodeData->children.push_back(child);
			}
		}

		return nodeData;
	}

	static NodeData* parseNodeRecursivelyBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, bool isSingleSprite, const std::string& version, NodeData* parent, bool& skeleton)
	{
		const std::string& id = binary.readString();

		size_t readCount = binary.read(&skeleton, 1, 1);
		if (readCount != 1)
		{
			Logger::log("warning: Failed to read is sleleton");
			return nullptr;
		}

		Mat4 transform;
		bool success = binary.readMatrix((float*)transform.m);
		if (!success)
		{
			Logger::log("warning: Failed to read transform matrix");
			return nullptr;
		}

		unsigned int partsSize;
		readCount = binary.read(&partsSize, 4, 1);
		if (readCount != 1)
		{
			Logger::log("warning: Failed to read nodedata: parts size");
			return nullptr;
		}

		NodeData* node = new (std::nothrow)NodeData();
		node->parent = parent;
		node->id = id;

		bool isSkin = false;

		if (partsSize > 0)
		{
			for (unsigned int i = 0; i < partsSize; ++i)
			{
				ModelData* model = new (std::nothrow)ModelData();
				model->subMeshId = binary.readString();
				if (model->subMeshId.empty())
				{
					Logger::log("part is missing meshPartId");
					delete model;
					delete node;
					return nullptr;
				}

				model->materialId = binary.readString();
				if (model->materialId.empty())
				{
					Logger::log("part is missing materialId");
					delete model;
					delete node;
					return nullptr;
				}

				unsigned int boneSize;
				readCount = binary.read(&boneSize, 4, 1);
				if (readCount != 1)
				{
					Logger::log("warning: Failed to read nodedata: bone size");
					delete model;
					delete node;
					return nullptr;
				}

				if (boneSize > 0)
				{
					for (unsigned int j = 0; j < boneSize; ++j)
					{
						model->bones.push_back(binary.readString());

						Mat4 invBindPose;
						success = binary.readMatrix((float*)invBindPose.m);
						if (!success)
						{
							Logger::log("warning: Failed to read nodedata: invBinsPose");
							delete model;
							delete node;
							return nullptr;
						}

						model->invBindPose.push_back(invBindPose);
					}

					isSkin = true;
				}

				unsigned int uvMapping;
				readCount = binary.read(&uvMapping, 4, 1);
				if (readCount != 1)
				{
					Logger::log("warning: Failed to read nodedata: uvMapping");
					delete model;
					delete node;
					return nullptr;
				}

				for (unsigned int j = 0; j < uvMapping; ++j)
				{
					unsigned int textureSize;
					readCount = binary.read(&textureSize, 4, 1);
					if (readCount != 1)
					{
						Logger::log("warning: Failed to read nodedata: uvMapping");
						delete model;
						delete node;
						return nullptr;
					}

					for (unsigned int k = 0; k < textureSize; ++k)
					{
						unsigned int index;
						readCount = binary.read(&index, 4, 1);
						if (readCount != 1)
						{
							Logger::log("warning: Failed to read nodedata: uvMapping");
							delete model;
							delete node;
							return nullptr;
						}
					}
				}

				node->modelNodeDatas.push_back(model);
			}
		}

		if (version == "0.1" || version == "0.2" || version == "0.3" || version == "0.4" || version == "0.5" || version == "0.6")
		{
			if (isSkin || isSingleSprite)
			{
				node->transform = Mat4::IDENTITY;
			}
			else
			{
				node->transform = transform;
			}
		}
		else
		{
			node->transform = transform;
		}

		unsigned int childrenSize;
		readCount = binary.read(&childrenSize, 4, 1);
		if (readCount != 1)
		{
			Logger::log("warning: Failed to read nodedata: uvMapping");
			delete node;
			return nullptr;
		}

		if (childrenSize > 0)
		{
			for (unsigned int i = 0; i < childrenSize; ++i)
			{
				NodeData* childNode = parseNodeRecursivelyBinary(binary, seekPointTable, isSingleSprite, version, node, skeleton);
				// nullptrが返っても加える
				node->children.push_back(childNode);
			}
		}

		return node;
	}

	static bool loadNodesFromJson(const rapidjson::Document& json, NodeDatas& outNodeDatas, const std::string& version)
	{
		if (!json.HasMember("nodes"))
		{
			return false;
		}

		const rapidjson::Value& nodesVal = json["nodes"];
		if (!nodesVal.IsArray())
		{
			return false;
		}

		rapidjson::SizeType nodesValSize = nodesVal.Size();
		for (rapidjson::SizeType i = 0; i < nodesValSize; ++i)
		{
			const rapidjson::Value& nodeVal = nodesVal[i];
			NodeData* nodeData = parseNodeRecursivelyJson(nodeVal, nodesValSize == 1, version, nullptr);
			// nullptrが返っても加える

			bool isSkelton = nodeVal["skeleton"].GetBool();
			if (isSkelton)
			{
				outNodeDatas.skeleton.push_back(nodeData);
			}
			else
			{
				outNodeDatas.nodes.push_back(nodeData);
			}
		}

		return true;
	}

	static bool loadNodesFromBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, NodeDatas& outNodeDatas, const std::string& version)
	{
		bool success = seek(binary, seekPointTable, SeekDataType::NODE);
		if (!success)
		{
			return "seek failed, SeekDataType::NODE";
		}

		unsigned int nodeSize;
		size_t readCount = binary.read(&nodeSize, 4, 1);
		if (readCount != 1)
		{
			return "warning: Failed to read nodes";
		}

		for (unsigned int i = 0; i < nodeSize; ++i)
		{
			bool skeleton;
			NodeData* node = parseNodeRecursivelyBinary(binary, seekPointTable, nodeSize == 1, version, nullptr, skeleton);
			// nullptrが返っても加える

			if (skeleton)
			{
				outNodeDatas.skeleton.push_back(node);
			}
			else
			{
				outNodeDatas.nodes.push_back(node);
			}
		}

		return "";
	}

	static bool loadAnimationDataFromJson(const rapidjson::Document& json, AnimationDatas& outAnimationDatas, const std::string& version)
	{
		std::string anim;
		if (version == "1.2" || version == "0.2")
		{
			anim = "animation";
		}
		else
		{
			anim = "animations";
		}

		if (!json.HasMember(anim.c_str()))
		{
			return false;
		}

		const rapidjson::Value& animationsVal = json[anim.c_str()];
		rapidjson::SizeType animationsValSize = animationsVal.Size();

		// タイムラインラベルごと
		for (rapidjson::SizeType i = 0; i < animationsValSize; ++i)
		{
			const rapidjson::Value& animationVal = animationsVal[i];

			AnimationData* animationData = new AnimationData();
			animationData->totalTime = animationVal["length"].GetDouble();

			const rapidjson::Value& bonesVal = animationVal["bones"];
			rapidjson::SizeType bonesValSize = bonesVal.Size();
			for (rapidjson::SizeType j = 0; j < bonesValSize; ++j)
			{
				const rapidjson::Value& boneVal = bonesVal[j];
				if (!boneVal.HasMember("keyframes"))
				{
					continue;
				}

				const rapidjson::Value& keyframesVal = boneVal["keyframes"];
				rapidjson::SizeType keyframesValSize = keyframesVal.Size();

				const std::string& boneName = boneVal["boneId"].GetString(); // なぜかここだけキー名が大文字
				animationData->translationKeyFrames[boneName].reserve(keyframesValSize);
				animationData->rotationKeyFrames[boneName].reserve(keyframesValSize);
				animationData->scaleKeyFrames[boneName].reserve(keyframesValSize);

				for (rapidjson::SizeType k = 0; k < keyframesValSize; ++k)
				{
					const rapidjson::Value& keyframeVal = keyframesVal[k];

					if (keyframeVal.HasMember("translation"))
					{
						const rapidjson::Value& translationVal = keyframeVal["translation"];
						float time = keyframeVal["keytime"].GetDouble();
						Vec3 translation(translationVal[(rapidjson::SizeType)0].GetDouble(), translationVal[(rapidjson::SizeType)1].GetDouble(), translationVal[(rapidjson::SizeType)2].GetDouble());
						animationData->translationKeyFrames[boneName].push_back(AnimationData::Vec3KeyFrame(time, translation));
					}

					if (keyframeVal.HasMember("rotation"))
					{
						const rapidjson::Value& rotationVal = keyframeVal["rotation"];
						float time = keyframeVal["keytime"].GetDouble();
						Quaternion rotation(rotationVal[(rapidjson::SizeType)0].GetDouble(), rotationVal[(rapidjson::SizeType)1].GetDouble(), rotationVal[(rapidjson::SizeType)2].GetDouble(), rotationVal[(rapidjson::SizeType)3].GetDouble());
						animationData->rotationKeyFrames[boneName].push_back(AnimationData::QuaternionKeyFrame(time, rotation));
					}

					if (keyframeVal.HasMember("scale"))
					{
						const rapidjson::Value& scaleVal = keyframeVal["scale"];
						float time = keyframeVal["keytime"].GetDouble();
						Vec3 scale(scaleVal[(rapidjson::SizeType)0].GetDouble(), scaleVal[(rapidjson::SizeType)1].GetDouble(), scaleVal[(rapidjson::SizeType)2].GetDouble());
						animationData->scaleKeyFrames[boneName].push_back(AnimationData::Vec3KeyFrame(time, scale));
					}
				}
			}

			const std::string& timelineName = animationVal["id"].GetString();
			outAnimationDatas.animations[timelineName] = animationData;
		}

		return true;
	}

	static bool loadAnimationDataFromBinary(BinaryReader& binary, const std::vector<SeekData>& seekPointTable, AnimationDatas& outAnimationDatas, const std::string& version)
	{
		if (version == "0.1" || version == "0.2" || version == "0.3" || version == "0.4")
		{
			bool success = seek(binary, seekPointTable, SeekDataType::ANIMATIONS);
			if (!success)
			{
				Logger::log("seek failed: animations");
				return false;
			}
		}
		else
		{
			//TODO: こっちはアニメラベルでseekせねばならず、すべて読み取るというのができない
			// やるには、seekでtypeすべてをループで取り出すようにする必要があるが、それをやるなら、
			// ラベル指定で取り出すつくりにしたほうが早い
			Logger::logAssert(false, "現状このバージョンには非対応ver=%s", version.c_str());
			return false;
		}

		size_t readCount;
		unsigned int numAnim = 1;
		if (version == "0.3" || version == "0.4")
		{
			readCount = binary.read(&numAnim, 4, 1);
			if (readCount != 1)
			{
				Logger::log("warning: Failed to read AnimationData: animNum");
				return false;
			}
		}

		for (unsigned int i = 0; i < numAnim; ++i)
		{
			AnimationData* animation = new (std::nothrow)AnimationData();

			const std::string& animId = binary.readString();

			readCount = binary.read(&animation->totalTime, 4, 1);
			if (readCount != 1)
			{
				Logger::log("warning: Failed to read AnimationData: totalTime");
				delete animation;
				outAnimationDatas.resetData();
				return false;
			}

			unsigned int numNodeAnimation;
			readCount = binary.read(&numNodeAnimation, 4, 1);
			if (readCount != 1)
			{
				Logger::log("warning: Failed to read AnimationData: num anim");
				delete animation;
				outAnimationDatas.resetData();
				return false;
			}

			for (unsigned int j = 0; j < numNodeAnimation; ++j)
			{
				const std::string& boneName = binary.readString();

				unsigned int numKeyFrame;
				readCount = binary.read(&numKeyFrame, 4, 1);
				if (readCount != 1)
				{
					Logger::log("warning: Failed to read AnimationData: num keyframe");
					delete animation;
					outAnimationDatas.resetData();
					return false;
				}

				animation->rotationKeyFrames[boneName].reserve(numKeyFrame);
				animation->scaleKeyFrames[boneName].reserve(numKeyFrame);
				animation->translationKeyFrames[boneName].reserve(numKeyFrame);

				for (unsigned int k = 0; k < numKeyFrame; ++k)
				{
					float keyTime;
					readCount = binary.read(&keyTime, 4, 1);
					if (readCount != 1)
					{
						Logger::log("warning: Failed to read AnimationData: keytime");
						delete animation;
						outAnimationDatas.resetData();
						return false;
					}

					// transformFlag
					unsigned char transformFlag(0);
					if (version != "0.1" && version != "0.2" && version != "0.3")
					{
						readCount = binary.read(&transformFlag, 1, 1);
						if (readCount != 1)
						{
							Logger::log("warning: Failed to read AnimationData: transformFlag");
							delete animation;
							outAnimationDatas.resetData();
							return false;
						}
					}

					// rotation
					bool hasRotate = true;
					if (version != "0.1" && version != "0.2" && version != "0.3")
					{
						hasRotate = transformFlag & 0x01;
					}

					if (hasRotate)
					{
						Quaternion rotate;
						readCount = binary.read(&rotate, 4, 4);
						if (readCount != 4)
						{
							Logger::log("warning: Failed to read AnimationData: rotate");
							delete animation;
							outAnimationDatas.resetData();
							return false;
						}

						animation->rotationKeyFrames[boneName].push_back(AnimationData::QuaternionKeyFrame(keyTime, rotate));
					}

					// rotation
					bool hasScale = true;
					if (version != "0.1" && version != "0.2" && version != "0.3")
					{
						hasScale = (transformFlag >> 1) & 0x01;
					}

					if (hasScale)
					{
						Vec3 scale;
						readCount = binary.read(&scale, 4, 3);
						if (readCount != 3)
						{
							Logger::log("warning: Failed to read AnimationData: scale");
							delete animation;
							outAnimationDatas.resetData();
							return false;
						}

						animation->scaleKeyFrames[boneName].push_back(AnimationData::Vec3KeyFrame(keyTime, scale));
					}

					// translation
					bool hasTranslation = true;
					if (version != "0.1" && version != "0.2" && version != "0.3")
					{
						hasTranslation = (transformFlag >> 2) & 0x01;
					}

					if (hasTranslation)
					{
						Vec3 translation;
						readCount = binary.read(&translation, 4, 3);
						if (readCount != 3)
						{
							Logger::log("warning: Failed to read AnimationData: translation");
							delete animation;
							outAnimationDatas.resetData();
							return false;
						}

						animation->translationKeyFrames[boneName].push_back(AnimationData::Vec3KeyFrame(keyTime, translation));
					}
				}
			}

			outAnimationDatas.animations[animId] = animation;
		}

		return true;
	}

	std::string loadC3t(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas)
	{
		const std::string& json = FileUtility::getInstance()->getStringFromFile(fileName);

		rapidjson::Document jsonReader;
		auto& doc = jsonReader.ParseInsitu<0>((char*)json.c_str()); // 型の扱いが難しいのでautoで扱う
		if (doc.HasParseError())
		{
			return "Parse json failed.";
		}

		//TODO:ここはよくわからないので写経してるだけ。後で調べよう
		const rapidjson::Value& mashDataArray = jsonReader["version"];
		std::string c3tVersion;
		if (mashDataArray.IsArray())
		{
			c3tVersion = "1.2";
		}
		else
		{
			c3tVersion = mashDataArray.GetString();
		}

		// meshデータロード
		if (c3tVersion == "1.2" || c3tVersion == "0.2")
		{
			loadMeshDatasFromJson_0_1(jsonReader, outMeshDatas);
		}
		else
		{
			loadMeshDatasFromJson(jsonReader, outMeshDatas);
		}

		ssize_t lastSlashIndex = fileName.find_last_of('/');
		const std::string& dirPath = fileName.substr(0, lastSlashIndex + 1);
		
		// materialデータロード
		if (c3tVersion == "1.2")
		{
			loadMaterialDatasFromJson_0_1(jsonReader, dirPath, outMaterialDatas);
		}
		else if (c3tVersion == "0.2")
		{
			loadMaterialDatasFromJson_0_2(jsonReader, dirPath, outMaterialDatas);
		}
		else
		{
			loadMaterialDatasFromJson(jsonReader, dirPath, outMaterialDatas);
		}
		
		// nodeデータロード
		if (c3tVersion == "0.1" || c3tVersion == "0.2" || c3tVersion == "1.2")
		{
			SkinData skinData;
			// TODO:orc.c3tにはskin要素はないようだしloadSkinは割愛しようかな。。
			bool loaded = loadSkinDataFromJson(jsonReader, skinData);
			if (!loaded)
			{
				NodeData* node = new (std::nothrow)NodeData();
				ModelData* model = new (std::nothrow)ModelData();
				model->materialId = "";
				model->subMeshId = "";
				node->modelNodeDatas.push_back(model);
				outNodeDatas.nodes.push_back(node);
				return "";
			}

			NodeData** nodeDatas = new (std::nothrow)NodeData*[skinData.skinBoneNames.size() + skinData.nodeBoneNames.size()];
			int index = 0;

			size_t skinBoneNamesSize = skinData.skinBoneNames.size();
			for (size_t i = 0; i < skinBoneNamesSize; ++i)
			{
				nodeDatas[index] = new (std::nothrow)NodeData();
				nodeDatas[index]->id = skinData.skinBoneNames[i];
				nodeDatas[index]->transform = skinData.skinBoneOriginMatrices[i];
				index++;
			}

			size_t nodeBoneNamesSize = skinData.nodeBoneNames.size();
			for (size_t i = 0; i < nodeBoneNamesSize; ++i)
			{
				nodeDatas[index] = new (std::nothrow)NodeData();
				nodeDatas[index]->id = skinData.nodeBoneNames[i];
				nodeDatas[index]->transform = skinData.nodeBoneOriginMatrices[i];
				index++;
			}

			for (const std::pair<int, std::vector<int>>& it : skinData.boneChild)
			{
				const std::vector<int>& children = it.second;
				NodeData* parent = nodeDatas[it.first];

				for (int child : children)
				{
					parent->children.push_back(nodeDatas[child]);
				}
			}

			outNodeDatas.skeleton.push_back(nodeDatas[skinData.rootBoneIndex]);

			NodeData* node = new (std::nothrow)NodeData();
			ModelData* model = new (std::nothrow)ModelData();
			model->materialId = "";
			model->subMeshId = "";
			model->bones = skinData.skinBoneNames;
			model->invBindPose = skinData.inverseBindPoseMatrices;
			node->modelNodeDatas.push_back(model);
			outNodeDatas.nodes.push_back(node);
			delete[] nodeDatas;
		}
		else
		{
			loadNodesFromJson(jsonReader, outNodeDatas, c3tVersion);
		}
		
		loadAnimationDataFromJson(jsonReader, outAnimationDatas, c3tVersion);
		return "";
	}

	std::string loadC3b(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas)
	{
		size_t length;
		unsigned char* buffer = FileUtility::getInstance()->getFileData(fileName, &length);
		if (buffer == nullptr || length <= 0)
		{
			return "empty file";
		}

		BinaryReader binaryReader;
		binaryReader.init(buffer, length);

		// シグネチャ確認
		char identifier[] = {'C', '3', 'B', '\0'};
		char sig[4];
		size_t readCount = binaryReader.read(sig, 1, 4);
		if (readCount != 4 || memcmp(sig, identifier, 4) != 0)
		{
			return "warning: Invalid identifier.";
		}

		// バージョン取得
		char version[2];
		readCount = binaryReader.read(version, 1, 2);
		if (readCount != 2)
		{
			return "warning: Failed to read version";
		}

		char versionStr[20]; // TODO:20の根拠は不明
		sprintf_s(versionStr, "%d.%d", version[0], version[1]);
		std::string c3bVersion(versionStr);

		// この後はシーク用データテーブル（id、type、offset）が続く。その後、実データ。

		// テーブルのサイズ取得
		unsigned int seekPointCount = 0;
		readCount = binaryReader.read(&seekPointCount, 4, 1);
		if (readCount != 1)
		{
			return "warning: Failed to read ref table size";
		}

		// シーク用データテーブル作成
		std::vector<SeekData> seekPointTable(seekPointCount);
		for (unsigned int i = 0; i < seekPointCount; ++i)
		{
			seekPointTable[i].id = binaryReader.readString();
			if (seekPointTable[i].id.empty())
			{
				Logger::log("warning: Failed to read id");
				break;
			}

			readCount = binaryReader.read(&seekPointTable[i].type, 4, 1);
			if (readCount != 1)
			{
				Logger::log("warning: Failed to read type");
				break;
			}

			readCount = binaryReader.read(&seekPointTable[i].offset, 4, 1);
			if (readCount != 1)
			{
				Logger::log("warning: Failed to read type");
				break;
			}
		}

		// meshデータロード
		if (c3bVersion == "0.1" || c3bVersion == "0.2")
		{
			loadMeshDatasFromBinary_0_1(binaryReader, seekPointTable, outMeshDatas);
		}
		else
		{
			loadMeshDatasFromBinary(binaryReader, seekPointTable, outMeshDatas);
		}

		ssize_t lastSlashIndex = fileName.find_last_of('/');
		const std::string& dirPath = fileName.substr(0, lastSlashIndex + 1);
		
		// materialデータロード
		if (c3bVersion == "0.1")
		{
			loadMaterialDatasFromBinary_0_1(binaryReader, seekPointTable, dirPath, outMaterialDatas);
		}
		else if (c3bVersion == "0.2")
		{
			loadMaterialDatasFromBinary_0_2(binaryReader, seekPointTable, dirPath, outMaterialDatas);
		}
		else
		{
			loadMaterialDatasFromBinary(binaryReader, seekPointTable, dirPath, outMaterialDatas);
		}
		
		// nodeデータロード
		if (c3bVersion == "0.1" || c3bVersion == "0.2" || c3bVersion == "1.2")
		{
			SkinData skinData;
			// TODO:orc.c3tにはskin要素はないようだしloadSkinは割愛しようかな。。
			bool loaded = loadSkinDataFromBinary(binaryReader, seekPointTable, skinData);
			if (!loaded)
			{
				NodeData* node = new (std::nothrow)NodeData();
				ModelData* model = new (std::nothrow)ModelData();
				model->materialId = "";
				model->subMeshId = "";
				node->modelNodeDatas.push_back(model);
				outNodeDatas.nodes.push_back(node);
				return "";
			}

			NodeData** nodeDatas = new (std::nothrow)NodeData*[skinData.skinBoneNames.size() + skinData.nodeBoneNames.size()];
			int index = 0;

			size_t skinBoneNamesSize = skinData.skinBoneNames.size();
			for (size_t i = 0; i < skinBoneNamesSize; ++i)
			{
				nodeDatas[index] = new (std::nothrow)NodeData();
				nodeDatas[index]->id = skinData.skinBoneNames[i];
				nodeDatas[index]->transform = skinData.skinBoneOriginMatrices[i];
				index++;
			}

			size_t nodeBoneNamesSize = skinData.nodeBoneNames.size();
			for (size_t i = 0; i < nodeBoneNamesSize; ++i)
			{
				nodeDatas[index] = new (std::nothrow)NodeData();
				nodeDatas[index]->id = skinData.nodeBoneNames[i];
				nodeDatas[index]->transform = skinData.nodeBoneOriginMatrices[i];
				index++;
			}

			for (const std::pair<int, std::vector<int>>& it : skinData.boneChild)
			{
				const std::vector<int>& children = it.second;
				NodeData* parent = nodeDatas[it.first];

				for (int child : children)
				{
					parent->children.push_back(nodeDatas[child]);
				}
			}

			outNodeDatas.skeleton.push_back(nodeDatas[skinData.rootBoneIndex]);

			NodeData* node = new (std::nothrow)NodeData();
			ModelData* model = new (std::nothrow)ModelData();
			model->materialId = "";
			model->subMeshId = "";
			model->bones = skinData.skinBoneNames;
			model->invBindPose = skinData.inverseBindPoseMatrices;
			node->modelNodeDatas.push_back(model);
			outNodeDatas.nodes.push_back(node);
			delete[] nodeDatas;
		}
		else
		{
			loadNodesFromBinary(binaryReader, seekPointTable, outNodeDatas, c3bVersion);
		}
		
		loadAnimationDataFromBinary(binaryReader, seekPointTable, outAnimationDatas, c3bVersion);

		return "";
	}
} // namespace C3bLoader

} // namespace mgrrenderer
