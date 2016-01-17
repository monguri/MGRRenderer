#pragma once
#include <string>
#include <vector>
#include <map>
#include "FileUtility.h"
#include "BasicDataTypes.h"


namespace mgrrenderer
{

namespace ObjLoader
{
	struct MeshData
	{
		std::string name;
		//TODO:åªèÛÇ≈ÇÕvnÇ…ÇÕÇ‹ÇæëŒâûÇµÇ»Ç¢
		std::vector<Position3DNormalTextureCoordinates> vertices;
		std::vector<unsigned short> indices;
		std::vector<int> materialIndices;
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
		int illumination;

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
			illumination(0)
		{}
	};

	std::string loadObj(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray);
} // namespace ObjLoader

} // namespace mgrrenderer
