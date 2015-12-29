#include "C3bLoader.h"
#include "FileUtility.h"
#include "external/json/document.h"

namespace mgrrenderer
{

namespace C3bLoader
{
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
			assert(false);
			printf("Invalid GL str.");
			return 0;
		}
	}

	static AttributeLocation parseGLProgramAttributeString(const std::string& str)
	{
		if (str == "VERTEX_ATTRIB_POSITION")
		{
			return AttributeLocation::POSITION;
		}
		else if (str == "VERTEX_ATTRIB_COLOR")
		{
			return AttributeLocation::COLOR;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD")
		{
			return AttributeLocation::TEXTURE_COORDINATE;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD1")
		{
			return AttributeLocation::TEXTURE_COORDINATE_1;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD2")
		{
			return AttributeLocation::TEXTURE_COORDINATE_2;
		}
		else if (str == "VERTEX_ATTRIB_TEX_COORD3")
		{
			return AttributeLocation::TEXTURE_COORDINATE_3;
		}
		else if (str == "VERTEX_ATTRIB_NORMAL")
		{
			return AttributeLocation::NORMAL;
		}
		else if (str == "VERTEX_ATTRIB_BLEND_WEIGHT")
		{
			return AttributeLocation::BLEND_WEIGHT;
		} 
		else if (str == "VERTEX_ATTRIB_BLEND_INDEX")
		{
			return AttributeLocation::BLEND_INDEX;
		}
		else
		{
			assert(false);
			printf("Wrong Attribute type.");
			return AttributeLocation::NONE;
		}
	}
	
	static TextureData::Usage parseGLTextureTypeString(const std::string& str)
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
			assert(false);
			printf("Wrong Texture type");
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
			attrib.type = parseGLTypeString(type);
			attrib.location = parseGLProgramAttributeString(attribute);
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
				attrib.type = parseGLTypeString(type);
				attrib.location = parseGLProgramAttributeString(attribute);
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
				texture.type = parseGLTextureTypeString(textureVal["type"].GetString());
				texture.wrapS = parseGLTypeString(textureVal["wrapModeU"].GetString());
				texture.wrapT = parseGLTypeString(textureVal["wrapModeV"].GetString());
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

	static bool loadSkinDataJson(const rapidjson::Document& json, SkinData& outSkinData)
	{
		if (!json.HasMember("skin"))
		{
			return false;
		}

		const rapidjson::Value& skinDataArrayVal = json["skin"];
		assert(skinDataArrayVal.IsArray());

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
					printf("warning: Node %s part is missing meshPartId or materialId", nodeData->id.c_str());
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
							printf("warning: Bone node ID missing");
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
				nodeData->children.push_back(child);
			}
		}

		return nodeData;
	}

	static bool loadNodesJson(const rapidjson::Document& json, NodeDatas& outNodeDatas, const std::string& version)
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
			const std::string& id = nodeVal["id"].GetString();
			NodeData* nodeData = parseNodeRecursivelyJson(nodeVal, nodesValSize == 1, version, nullptr);

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

	static bool loadAnimationDataJson(const rapidjson::Document& json, AnimationDatas& outAnimationDatas, const std::string& version)
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

	std::string loadC3t(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas)
	{
		const std::string& json = FileUtility::getInstance()->getStringFromFile(fileName);

		rapidjson::Document jsonReader;
		auto& doc = jsonReader.ParseInsitu<0>((char*)json.c_str()); // 型の扱いが難しいのでautoで扱う
		if (doc.HasParseError())
		{
			assert(false);
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
			bool loaded = loadSkinDataJson(jsonReader, skinData);
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
			for (size_t i = 0; i < skinBoneNamesSize; ++i)
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
			loadNodesJson(jsonReader, outNodeDatas, c3tVersion);
		}
		
		loadAnimationDataJson(jsonReader, outAnimationDatas, c3tVersion);
		return "";
	}

	std::string loadC3b(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas)
	{

		return "";
	}
} // namespace C3bLoader

} // namespace mgrrenderer
