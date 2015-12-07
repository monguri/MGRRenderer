#pragma once
#include <string>
#include <vector>
#include <map>
#include "BasicDataTypes.h"

namespace mgrrenderer
{

namespace C3bLoader
{
	struct MeshVertexAttribute
	{
		// attribute size
		GLint size;
		// ex. GL_FLOAT
		GLenum type;
		//VERTEX_ATTRIB_POSITION,VERTEX_ATTRIB_COLOR,VERTEX_ATTRIB_TEX_COORD,VERTEX_ATTRIB_NORMAL, VERTEX_ATTRIB_BLEND_WEIGHT, VERTEX_ATTRIB_BLEND_INDEX, GLProgram for detail
		AttributeLocation location;
		//size in bytes
		int attributeSizeBytes;
	};

	struct MeshData
	{
		typedef std::vector<unsigned short> IndexArray;
		std::vector<float> vertices;
		size_t vertexSizeInFloat;
		std::vector<IndexArray> subMeshIndices;
		std::vector<std::string> subMeshIds;
		//std::vector<AABB> subMeshAABB;
		size_t numSubMesh;
		std::vector<MeshVertexAttribute> attributes;
		size_t numAttribute;
	};

	struct MeshDatas
	{
		std::vector<MeshData*> meshDatas;

		~MeshDatas()
		{
			resetData();
		}

		void resetData()
		{
			for (const auto& it : meshDatas)
			{
				delete it;
			}
			meshDatas.clear();
		}
	};

	struct TextureData
	{
		enum class Usage
		{
			UNKNOWN = 0,
			NONE,
			DIFFUSE,
			EMISSIVE,
			AMBIENT,
			SPECULAR,
			SHININESS,
			NORMAL,
			BUMP,
			TRANSPARENCY,
			REFLECTION,
		};

		std::string id;
		std::string fileName;
		Usage type;
		GLenum wrapS;
		GLenum wrapT;
	};

	struct MaterialData
	{
		std::vector<TextureData> textures;
		std::string id;
	};

	struct MaterialDatas
	{
		std::vector<MaterialData*> materialDatas;

		~MaterialDatas()
		{

		}

		void resetData()
		{
			for (const auto& it : materialDatas)
			{
				delete it;
			}
			materialDatas.clear();
		}
	};

	struct SkinData
	{
		std::vector<std::string> skinBoneNames; //skin bones affect skin
		std::vector<std::string> nodeBoneNames; //node bones don't affect skin, all bones [skinBone, nodeBone]
		std::vector<Mat4> inverseBindPoseMatrices; //bind pose of skin bone, only for skin bone
		std::vector<Mat4> skinBoneOriginMatrices; // original bone transform, for skin bone
		std::vector<Mat4> nodeBoneOriginMatrices; // original bone transform, for node bone

		//bone child info, both skinbone and node bone
		std::map<int, std::vector<int>> boneChild; //key parent, value child
		int rootBoneIndex = -1;

		void addSkinBoneName(const std::string& name)
		{
			if (std::find(skinBoneNames.begin(), skinBoneNames.end(), name) == skinBoneNames.end())
			{
				skinBoneNames.push_back(name);
			}
		}

		void addNodeBoneName(const std::string& name)
		{
			if (std::find(nodeBoneNames.begin(), nodeBoneNames.end(), name) == nodeBoneNames.end())
			{
				nodeBoneNames.push_back(name);
			}
		}

		int getSkinBoneNameIndex(const std::string& name) const
		{
			int i = 0;
			for (const std::string& skinBoneName : skinBoneNames)
			{
				if (name == skinBoneName)
				{
					return i;
				}
				++i;
			}

			return -1;
		}

		int getBoneNameIndex(const std::string& name) const
		{
			int i = 0;
			for (const std::string& skinBoneName : skinBoneNames)
			{
				if (name == skinBoneName)
				{
					return i;
				}
				++i;
			}

			for (const std::string& nodeBoneName : nodeBoneNames)
			{
				if (name == nodeBoneName)
				{
					return i;
				}
				++i;
			}

			return -1;
		}
	};

	// TODO:–{“–‚É‚¢‚é‚Ì‚©‚È‚ ‚±‚ê
	struct ModelData
	{
		std::string subMeshId;
		std::string materialId;
		std::vector<std::string> bones;
		std::vector<Mat4> invBindPose;

		void resetData()
		{
			bones.clear();
			invBindPose.clear();
		}
	};

	struct NodeData
	{
		std::string id;
		Mat4 transform;
		std::vector<ModelData*> modelNodeDatas;
		std::vector<NodeData*> children;
		
		~NodeData()
		{
			resetData();
		}

		void resetData()
		{
			id.clear();
			transform.setZero();

			for (const auto& it : children)
			{
				delete it;
			}
			children.clear();

			for (const auto& it : modelNodeDatas)
			{
				delete it;
			}
			modelNodeDatas.clear();
		}
	};

	struct NodeDatas
	{
		std::vector<NodeData*> skeleton;
		std::vector<NodeData*> nodes;

		~NodeDatas()
		{
			resetData();
		}

		void resetData()
		{
			for (const auto& it : skeleton)
			{
				delete it;
			}
			skeleton.clear();

			for (const auto& it : nodes)
			{
				delete it;
			}
			nodes.clear();
		}
	};
	std::string loadC3t(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas);

	std::string loadC3b(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas);
} // namespace C3bLoader

} // namespace mgrrenderer
