#pragma once
#include <string>
#include <vector>
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
		std::vector<IndexArray> subMeshIndices;
		std::vector<std::string> subMeshIds;
		//std::vector<AABB> subMeshAABB;
		int numIndex;
		std::vector<MeshVertexAttribute> attributes;
		int numAttribute;
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
			REFRECTION,
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
		std::string id; // id?ファイル名？
	};

	std::string loadC3t(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray);

	std::string loadC3b(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray);
} // namespace C3bLoader

} // namespace mgrrenderer
