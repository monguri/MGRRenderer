#pragma once
#include <string>
#include <vector>
#include <map>
#include "utility/FileUtility.h"
#include "renderer/BasicDataTypes.h"


namespace mgrrenderer
{

namespace ObjLoader
{
	struct MeshData
	{
		typedef std::vector<unsigned short> IndexArray;
		std::string name;
		std::vector<Position3DNormalTextureCoordinates> vertices;
		std::vector<unsigned short> indices;
		std::vector<int> materialIndices;
		size_t numMaterialIndex;
		std::map<int, std::vector<unsigned short>> subMeshMap;
		// cocos2d-xではsubMeshMapをさらに分解してsubMeshIndicesとsumMeshIdsをもち、サブメッシュとマテリアルの対応は
		// ModelDataで保持しているが、別のデータを使うのも面倒なので分解しないことにした
		//std::vector<IndexArray> subMeshIndices;
		//std::vector<std::string> subMeshIds;
		//std::vector<AABB> subMeshAABB;
	};

	struct MaterialData
	{
		std::string name;
		Vec3 ambient;
		Vec3 diffuse;
		Vec3 specular;
		Vec3 transmittance;
		Vec3 emission;
		float shinness;
		float indexOfRefraction;
		float dissolve;
		float illumination;

		std::string ambientTextureName;
		std::string diffuseTextureName;
		std::string specularTextureName;
		std::string normalTextureName;
		std::map<std::string, std::string> unknownParameter;

		MaterialData() :
			name(""),
			ambientTextureName(""),
			diffuseTextureName(""),
			specularTextureName(""),
			normalTextureName(""),
			shinness(1.0f),
			indexOfRefraction(1.0f),
			dissolve(1.0f),
			illumination(0.0f)
		{}
	};

	std::string loadObj(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray);
} // namespace ObjLoader

} // namespace mgrrenderer
