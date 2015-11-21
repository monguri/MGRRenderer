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
//enum class AttributeLocation : int
//{
//	NONE = -1,
//
//	POSITION,
//	COLOR,
//	TEXTURE_COORDINATE,
//	TEXTURE_COORDINATE_1,
//	TEXTURE_COORDINATE_2,
//	TEXTURE_COORDINATE_3,
//	NORMAL,
//	BLEND_WEIGHT,
//	BLEND_INDEX,
//
//	NUM_ATTRIBUTE_IDS,
//};

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

	static std::string loadMeshDatasFromJson(rapidjson::Document json, std::vector<MeshData>& outMeshArray)
	{
		const rapidjson::Value& meshesVal = json["meshes"];
		rapidjson::SizeType meshesValSize = meshesVal.Size();
		for (rapidjson::SizeType i = 0; i < meshesValSize; ++i)
		{
			MeshData* mesh = new (std::nothrow) MeshData();
			const rapidjson::Value& meshVal = meshesVal[i];

			const rapidjson::Value& attributesVal = meshVal["attributes"];

			MeshVertexAttribute attrib;
			mesh->numAttribute = attributesVal.Size();
			mesh->attributes.resize(mesh->numAttribute);

			for (rapidjson::SizeType j = 0; j < meshesValSize; ++j)
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
		}

	}

	std::string loadC3t(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray)
	{
		const std::string& json = FileUtility::getInstance()->getStringFromFile(fileName);
		rapidjson::Document jsonReader;
		//jsonReader.ParseInsitu<0>(json.c_str());
		auto& doc = jsonReader.ParseInsitu<0>((char*)json.c_str()); // å^ÇÃàµÇ¢Ç™ìÔÇµÇ¢ÇÃÇ≈autoÇ≈àµÇ§
		if (doc.HasParseError())
		{
			assert(false);
			return "Parse json failed.";
		}

		//TODO:Ç±Ç±ÇÕÇÊÇ≠ÇÌÇ©ÇÁÇ»Ç¢ÇÃÇ≈é åoÇµÇƒÇÈÇæÇØÅBå„Ç≈í≤Ç◊ÇÊÇ§
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

		if (c3tVersion == "1.2" || c3tVersion == "0.2")
		{

		}
		else
		{

		}

		return "";
	}

	std::string loadC3b(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray)
	{

		return "";
	}
} // namespace C3bLoader

} // namespace mgrrenderer
