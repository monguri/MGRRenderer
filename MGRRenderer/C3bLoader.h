#pragma once
#include <string>
#include <vector>
#include <map>
#include "BasicDataTypes.h"

#if defined(MGRRENDERER_USE_OPENGL)
namespace mgrrenderer
{

enum class AttributeLocation : int;

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
		std::string id;
		std::vector<TextureData> textures;
		Color3F ambient;
		Color3F diffuse;
		Color3F specular;
		float shininess;
		Color3F emissive;
		float opacity;
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
		NodeData* parent; // TODO:cocos�ɂ͂Ȃ����Acocos��Skeleton3D��Bone3D�ɂ͂����āA�c���[�̃g���o�[�X�ɕK�v�Ȃ��߁A�b��I�ɒǉ�
		Mat4 animatedTransform; // TODO:cocos�ɂ͂Ȃ����Acocos��Skeleton3D��Bone3D�ɂ͂����āA�g���o�[�X���ăA�j���[�V�����s��p���b�g�����߂�̂ɕK�v�Ȃ��߁A�b��I�ɒǉ� Mat4::ZERO���A�A�j���[�V�������ĂȂ����̂Ƃ���transform�����g�����߂̔���l�Ɏg��

		NodeData() : parent(nullptr), transform(Mat4::ZERO), animatedTransform(Mat4::ZERO) {}
		
		~NodeData()
		{
			resetData();
		}

		void resetData()
		{
			id.clear();
			transform.setZero();
			animatedTransform.setZero();

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

	// ��̃^�C�����C���̃��x���ɑΉ�����
	struct AnimationData {
		// time��0<=time<=1�̃p�����[�^�ł���BtotalTime�Ə�Z���邱�ƂŎ����i�b�j�ɂȂ�
		struct Vec3KeyFrame {
			float time;
			Vec3 value;

			Vec3KeyFrame(float timeVal, const Vec3& val) : time(timeVal), value(val) {}
		};

		struct QuaternionKeyFrame {
			float time;
			Quaternion value;

			QuaternionKeyFrame(float timeVal, const Quaternion& val) : time(timeVal), value(val) {}
		};

		// �m�[�h��ID�ƃL�[�t���[���f�[�^�̔z��̃}�b�v
		std::map<std::string, std::vector<Vec3KeyFrame>> translationKeyFrames;
		std::map<std::string, std::vector<QuaternionKeyFrame>> rotationKeyFrames;
		std::map<std::string, std::vector<Vec3KeyFrame>> scaleKeyFrames;
		float totalTime;

		int determineIndex(const std::string& boneId, float time)
		{
			const std::vector<Vec3KeyFrame>& oneTranslationKeyFrames = translationKeyFrames[boneId];

			// 2���T���ōł��߂��C���f�b�N�X��T��
			unsigned int min = 0;
			unsigned int max = oneTranslationKeyFrames.size() - 1; // �����o�ϐ��̃x�N�g����
			unsigned int mid = 0;

			do
			{
				mid = (min + max) / 2;
				if (oneTranslationKeyFrames[mid].time <= time && time <= oneTranslationKeyFrames[mid + 1].time)
				{
					return mid;
				}
				else if (time < oneTranslationKeyFrames[mid].time)
				{
					max = mid - 1;
				}
				else
				{
					min = mid + 1;
				}
			} while (min <= max);

			// �q�b�g������̂Ȃ�
			return -1;
		}

		Vec3 evaluateTranslation(const std::string& boneId, float time)
		{
			const std::vector<Vec3KeyFrame>& oneTranslationKeyFrames = translationKeyFrames[boneId];

			if (oneTranslationKeyFrames.size() == 1 || time <= oneTranslationKeyFrames[0].time)
			{
				return oneTranslationKeyFrames[0].value;
			}
			else if (time >= oneTranslationKeyFrames[oneTranslationKeyFrames.size() - 1].time)
			{
				return oneTranslationKeyFrames[oneTranslationKeyFrames.size() - 1].value;
			}

			int index = determineIndex(boneId, time);

			// �⊮�Ɏg��2�̒l
			const Vec3KeyFrame& from = translationKeyFrames[boneId][index];
			const Vec3KeyFrame& to = translationKeyFrames[boneId][index + 1];

			// �⊮�p�����[�^
			float t = (time - from.time) / (from.time - to.time);
			return from.value + (to.value - from.value) * t;
		}

		Quaternion evaluateRotation(const std::string& boneId, float time)
		{
			const std::vector<QuaternionKeyFrame>& oneRotationKeyFrames = rotationKeyFrames[boneId];

			if (oneRotationKeyFrames.size() == 1 || time <= oneRotationKeyFrames[0].time)
			{
				return oneRotationKeyFrames[0].value;
			}
			else if (time >= oneRotationKeyFrames[oneRotationKeyFrames.size() - 1].time)
			{
				return oneRotationKeyFrames[oneRotationKeyFrames.size() - 1].value;
			}

			int index = determineIndex(boneId, time);

			// �⊮�Ɏg��2�̒l
			const QuaternionKeyFrame& from = rotationKeyFrames[boneId][index];
			const QuaternionKeyFrame& to = rotationKeyFrames[boneId][index + 1];

			// �⊮�p�����[�^
			float t = (time - from.time) / (to.time - from.time);
			if (t >= 0)
			{
				return Quaternion::slerp(from.value, to.value, t);
			}
			else
			{
				return Quaternion::slerp(to.value, from.value, t);
			}
		}

		Vec3 evaluateScale(const std::string& boneId, float time)
		{
			const std::vector<Vec3KeyFrame>& oneScaleKeyFrames = scaleKeyFrames[boneId];

			if (oneScaleKeyFrames.size() == 1 || time <= oneScaleKeyFrames[0].time)
			{
				return oneScaleKeyFrames[0].value;
			}
			else if (time >= oneScaleKeyFrames[oneScaleKeyFrames.size() - 1].time)
			{
				return oneScaleKeyFrames[oneScaleKeyFrames.size() - 1].value;
			}

			int index = determineIndex(boneId, time);

			// �⊮�Ɏg��2�̒l
			const Vec3KeyFrame& from = scaleKeyFrames[boneId][index];
			const Vec3KeyFrame& to = scaleKeyFrames[boneId][index + 1];

			// �⊮�p�����[�^
			float t = (time - from.time) / (from.time - to.time);
			return from.value + (to.value - from.value) * t;
		}
	};

	struct AnimationDatas {
		// �^�C�����C�����ƃf�[�^�̃}�b�v
		std::map<std::string, AnimationData*> animations;

		~AnimationDatas()
		{
			resetData();
		}

		void resetData()
		{
			for (const auto& it : animations)
			{
				delete it.second;
			}
			animations.clear();
		}
	};

	// TODO:����T�u�X���b�h����̃��[�h���l�����ĂȂ����l������悤�ɂ�����
	// �A�j���[�V�����f�[�^�͑傫���Ďg��Ȃ��ꍇ�Ƀ��[�h����̂����ʂȂ̂ŕʃ��\�b�h�Ń��[�h����
	std::string loadC3t(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas);

	std::string loadC3b(const std::string& fileName, MeshDatas& outMeshDatas, MaterialDatas& outMaterialDatas, NodeDatas& outNodeDatas, AnimationDatas& outAnimationDatas);
} // namespace C3bLoader

} // namespace mgrrenderer
#endif
