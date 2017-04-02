#include "ObjLoader.h"
#include <sstream>

namespace mgrrenderer
{

namespace ObjLoader
{
	static const size_t SSCANF_BUFFER_SIZE = 4096;

	struct VertexIndex {
		size_t vIdx;
		size_t vtIdx;
		size_t vnIdx;

		VertexIndex() {};
		VertexIndex(size_t idx) : vIdx(idx), vtIdx(idx), vnIdx(idx) {};
		VertexIndex(size_t vIdxVal, size_t vtIdxVal, size_t vnIdxVal) : vIdx(vIdxVal), vtIdx(vtIdxVal), vnIdx(vnIdxVal) {};

		// std::map�̃e���v���[�g�Ŏw�肷��^��operator<�̒�`���K�v
		bool operator<(const VertexIndex& another) const
		{
			// vIdx, vnIdx, vtIdx�̏��ɔ�r����
			if (vIdx != another.vIdx)
			{
				return vIdx < another.vIdx;
			}
			else if (vnIdx != another.vnIdx)
			{
				return vnIdx < another.vnIdx;
			}
			else if (vtIdx != another.vtIdx)
			{
				return vtIdx < another.vtIdx;
			}

			return false;
		}
	};

	static bool isSpace(const char c) { return (c == ' ' || c == '\t'); };
	static bool isNewLine(const char c) { return (c == '\r' || c == '\n' || c == '\0'); };

	static float parseFloat(const char*& token)
	{
		token += strspn(token, " \t");
		//const char* end = token + strcspn(token, " \t\r");
		// TODO:�Ȃ�cocos��atof���g��Ȃ��v�H+3.1417e+2�݂����ȋL�q�������Ă邩��H
		// TODO:monguri:�����Ԃ񂻂��B�����atof���g���`�ɏC�����悤
		// http://www.orchid.co.jp/computer/cschool/CREF/atof.html 
		//double val = parseDouble(token, end);
		//token = end;
		//return static_cast<float>(val);
		// TODO:cocos�ł͎��O��double���p�[�X���Ă����Ȃ����H
		size_t len = strcspn(token, " \t\r");
		std::string floatStr(token, len);
		token += len;
		float ret = static_cast<float>(atof(floatStr.c_str()));
		return ret;
	}

	static Vec2 parseVec2(const char*& token)
	{
		float x = parseFloat(token);
		float y = parseFloat(token);
		return Vec2(x, y);
	}

	static Vec3 parseVec3(const char*& token)
	{
		float x = parseFloat(token);
		float y = parseFloat(token);
		float z = parseFloat(token);
		return Vec3(x, y, z);
	}

	static std::string parseString(const char*& token)
	{
		std::string ret;
		token += strspn(token, " \t");
		size_t len = strcspn(token, " \t\r");
		ret = std::string(token, &token[len]); //TODO:����Atoken[len - 1]����Ȃ��́H�����āAtoken[len]����" \t\r"�̂ǂꂩ����Ȃ��́H
		token += len;
		return ret;
	}

	// Make index zero-base, and also support relative index.
	static size_t fixIndexForArray(int idx, int size)
	{
		if (idx > 0)
		{
			return static_cast<size_t>(idx - 1);
		}
		else if (idx == 0)
		{
			return 0;
		}
		else // idx < 0
		{
			Logger::logAssert(size + idx >= 0, "obj�t�@�C���ŃT�C�Y���傫�ȕ��̒l��������ꂽ�B");
			return static_cast<size_t>(size + idx); // relative idx
		}
	}

	// Parse triples: i, i/j/k, i//k, i/j
	static VertexIndex parseVertexIndex(const char*& token, int vsize, int vnsize, int vtsize)
	{
		VertexIndex ret;

		ret.vIdx = fixIndexForArray(atoi(token), vsize);
		token += strcspn(token, "/ \t\r");
		if (token[0] != '/')
		{
			return ret;
		}
		token++;

		// i//k
		if (token[0] == '/')
		{
			token++;
			ret.vnIdx = fixIndexForArray(atoi(token), vnsize);
			token += strcspn(token, "/ \t\r");
			return ret;
		}

		// i/j/k or i/j
		ret.vtIdx = fixIndexForArray(atoi(token), vtsize);
		token += strcspn(token, "/ \t\r");
		if (token[0] != '/')
		{
			return ret;
		}

		// i/j/k
		token++; // skip '/'
		ret.vnIdx = fixIndexForArray(atoi(token), vnsize);
		token += strcspn(token, "/ \t\r");
		return ret;
	}

	// �߂�l�́AMeshData.positions�ɓ��ꂽ�v�f�̃C���f�b�N�X�B�������AVertexIndex�����normals��texcoord�ɂ͗v�f������Ȃ��B
	// �L���b�V���ɂ��łɓ����Ă�����MeshData�ɂ͒ǉ������A���̃C���f�b�N�X��Ԃ��B
	static unsigned int addOneVetexDataToMeshData(
		std::map<VertexIndex, unsigned int> &vertexIndexPositionArrayIndexMap,
		MeshData& mesh,
		const std::vector<Vec3>& allPositions,
		const std::vector<Vec3>& allNormals,
		const std::vector<Vec2>& allTexCoords,
		const VertexIndex& vi
		) // static���\�b�h�ɂ��ăN���X�I�Ɉ���Ȃ��ƁA����p�͂Ȃ����ǂǂ����Ă�������������񂾂�ȁB�B
	{
		const std::map<VertexIndex, unsigned int>::iterator& it = vertexIndexPositionArrayIndexMap.find(vi);
		if (it != vertexIndexPositionArrayIndexMap.end())
		{
			// �L���b�V���Ƀq�b�g�����炻�̃C���f�b�N�X��Ԃ��B����ɂ���āA�C���f�b�N�X�`���ɂ��Ă���B
			return it->second;
		}

		Logger::logAssert(allPositions.size() > vi.vIdx, "�C���f�b�N�X�̒l���z�񐔂𒴂����B");
		Logger::logAssert(allNormals.size() > vi.vnIdx, "�C���f�b�N�X�̒l���z�񐔂𒴂����B");
		Logger::logAssert(allTexCoords.size() > vi.vtIdx, "�C���f�b�N�X�̒l���z�񐔂𒴂����B");

		Position3DNormalTextureCoordinates vertex;
		vertex.position = allPositions[vi.vIdx];

		if (vi.vnIdx >= 0) // face��normal�͕K������Ƃ͌���Ȃ�
		{
			vertex.normal = allNormals[vi.vnIdx];
		}
		else
		{
			Logger::logAssert(false, "����Avn�C���f�b�N�X�̏ȗ��ɂ͖��Ή�");
		}

		Logger::logAssert(vi.vtIdx >= 0, "����Avt�C���f�b�N�X�̏ȗ��ɂ͖��Ή�");
		vertex.textureCoordinate = allTexCoords[vi.vtIdx];

		mesh.vertices.push_back(vertex);

		unsigned int idxOfPositions = mesh.vertices.size() - 1;
		// �L���b�V���ɓ���Ă���
		vertexIndexPositionArrayIndexMap[vi] = idxOfPositions;
		return idxOfPositions;
	}

	static bool createMeshDataFromFaceGroup(
		MeshData& mesh,
		std::map<VertexIndex, unsigned int> vertexIndexPositionsArrayIndexMap,
		const std::vector<Vec3>& allPositions,
		const std::vector<Vec3>& allNormals,
		const std::vector<Vec2>& allTexCoords,
		const std::vector<std::vector<VertexIndex>>& faceGroup,
		const int materialIndex,
		const std::string& name,
		bool clearCache
		)
	{
		if (faceGroup.empty())
		{
			return false;
		}

		// �C���f�b�N�X���k���ꂽ�f�[�^��L������MeshData�����
		for (const std::vector<VertexIndex>& face : faceGroup)
		{
			// �|���S����TRIANGLE_FAN�`����3�p�`�ɕ������Ă����AMeshData��PNT�̔z��ɓ���Ă���
			// �ł�����vi0����Ă邩��A�f�[�^�̎������Ƃ��Ă�TRIANGLES�Ɠ����ŏd�����Ē��_�����Ă��

			// ���̃��[�v�̂��߂̏����l
			VertexIndex vi0 = face[0];
			VertexIndex vi1;
			VertexIndex vi2 = face[1];

			size_t numPolys = face.size();
			for (size_t i = 2; i < numPolys; i++)
			{
				vi1 = vi2;
				vi2 = face[i];

				unsigned int idx0 = addOneVetexDataToMeshData(
					vertexIndexPositionsArrayIndexMap,
					mesh,
					allPositions,
					allNormals,
					allTexCoords,
					vi0
					);
				mesh.indices.push_back(static_cast<unsigned short>(idx0));

				unsigned int idx1 = addOneVetexDataToMeshData(
					vertexIndexPositionsArrayIndexMap,
					mesh,
					allPositions,
					allNormals,
					allTexCoords,
					vi1
					);
				mesh.indices.push_back(static_cast<unsigned short>(idx1));

				unsigned int idx2 = addOneVetexDataToMeshData(
					vertexIndexPositionsArrayIndexMap,
					mesh,
					allPositions,
					allNormals,
					allTexCoords,
					vi2
					);
				mesh.indices.push_back(static_cast<unsigned short>(idx2));

				mesh.materialIndices.push_back(materialIndex); // �e���_���ƂɃ}�e���A��ID����������
				// TODO:����A�����z��Ɏ��������܂Ƃ߂�map�Ȃ�Ȃ�Ȃ�̃O���[�v�Ɏ������Ă������̂ł́B�܂��K������PNT���ׂăf�[�^������ƌ���Ȃ����烁�����e�ʓI�ɂ͖��ʂ����ǁB�B
			}
		}

		mesh.name = name;
		mesh.numMaterialIndex = mesh.materialIndices.size();

		if (clearCache)
		{
			vertexIndexPositionsArrayIndexMap.clear();
		}

		return true;
	}

	std::string loadMtl(const std::string& fileName, std::vector<MaterialData>& materials, std::map<std::string, int>& materialNameMaterialArrayIndexMap, const std::string& mtlBasePath)
	{
		std::string filePath;

		if (mtlBasePath.empty())
		{
			filePath = fileName;
		}
		else
		{
			filePath = mtlBasePath + fileName;
		}

		std::stringstream err;
		std::istringstream fstream(FileUtility::getInstance()->getStringFromFile(filePath));
		if (!fstream) //TODO: istringstrema�͉��^�Ȃ񂾁H
		{
			err << "Cannot open material file [" << filePath << "]" << std::endl;
			return err.str();
		}

		// Create a default material anyway.
		MaterialData material;

		int MAX_CHARS = 8192;
		std::vector<char> buf(MAX_CHARS);

		while (fstream.peek() != -1)
		{
			fstream.getline(&buf[0], MAX_CHARS);

			std::string lineBuf(&buf[0]);

			// Trim newline '\r\n' or '\n'
			size_t size = lineBuf.size();
			if (size > 0)
			{
				if (lineBuf[size - 1] == '\n')
				{
					lineBuf.erase(size - 1);
				}
			}
			size = lineBuf.size();
			if (size > 0)
			{
				if (lineBuf[size - 1] == '\r')
				{
					lineBuf.erase(size - 1);
				}
			}

			// Skip if empty line.
			if (lineBuf.empty())
			{
				continue;
			}

			// Skip leading space.
			const char* token = lineBuf.c_str();
			token += strspn(token, " \t");

			Logger::logAssert(token != nullptr, "�g�[�N���̎擾���s");

			if (token[0] == '\0')
			{
				continue; // empty line
			}

			if (token[0] == '#')
			{
				continue; // comment line
			}

			// new mtl
			if (strncmp(token, "newmtl", 6) == 0 && isSpace(token[6]))
			{
				// �����ł�obj��f�̂悤�ɁA�O��p�[�X�������̂�����ŏ������� newmtl�̌�ɂ���f�[�^���Ɋi�[���Ă���A������}�e���A�����ƌ��ѕt���A�z��Ɋi�[������
				if (!material.name.empty())
				{
					materialNameMaterialArrayIndexMap.insert(std::pair<std::string, int>(material.name, materials.size()));
					materials.push_back(material);
				}

				char nameBuf[SSCANF_BUFFER_SIZE];
				token += 7;
				sscanf_s(token, "%s", nameBuf, _countof(nameBuf));

				material = MaterialData();
				material.name = nameBuf;
			}
			// ambient
			else if (token[0] == 'K' && token[1] == 'a' && isSpace(token[2]))
			{
				token += 2;
				material.ambient = parseVec3(token);
			}
			// diffuse
			else if (token[0] == 'K' && token[1] == 'd' && isSpace(token[2]))
			{
				token += 2;
				material.diffuse = parseVec3(token);
			}
			// specular
			else if (token[0] == 'K' && token[1] == 's' && isSpace(token[2]))
			{
				token += 2;
				material.specular = parseVec3(token);
			}
			// transmittance
			else if (token[0] == 'K' && token[1] == 't' && isSpace(token[2]))
			{
				token += 2;
				material.transmittance = parseVec3(token);
			}
			// index of refraction
			else if (token[0] == 'N' && token[1] == 'i' && isSpace(token[2]))
			{
				token += 2;
				material.indexOfRefraction = parseFloat(token);
			}
			// emission
			else if (token[0] == 'K' && token[1] == 'e' && isSpace(token[2]))
			{
				token += 2;
				material.emission = parseVec3(token);
			}
			// shinness
			else if (token[0] == 'N' && token[1] == 's' && isSpace(token[2]))
			{
				token += 2;
				material.shinness = parseFloat(token);
			}
			// illumination model
			else if (strncmp(token, "illum", 5) == 0 && isSpace(token[5]))
			{
				token += 6;
				material.illumination = parseFloat(token);
			}
			// dissolve
			else if (token[0] == 'T' && token[1] == 'r' && isSpace(token[2]))
			{
				token += 2;
				material.dissolve = 1.0f - parseFloat(token);
			}
			// ambient texture
			else if (strncmp(token, "map_Ka", 6) == 0 && isSpace(token[6]))
			{
				token += 7;
				material.ambientTextureName = FileUtility::convertPathFormatToUnixStyle(token);
			}
			// diffuse texture
			else if (strncmp(token, "map_Kd", 6) == 0 && isSpace(token[6]))
			{
				token += 7;
				material.diffuseTextureName = FileUtility::convertPathFormatToUnixStyle(token);
			}
			// specular texture
			else if (strncmp(token, "map_Kd", 6) == 0 && isSpace(token[6]))
			{
				token += 7;
				material.specularTextureName = FileUtility::convertPathFormatToUnixStyle(token);
			}
			// normal texture
			else if (strncmp(token, "map_Ns", 6) == 0 && isSpace(token[6]))
			{
				token += 7;
				material.normalTextureName = FileUtility::convertPathFormatToUnixStyle(token);
			}
			// unknown parameter
			// TODO:monguri:loadObj�ł�unknown�ɂ͑Ή����ĂȂ������̂ɂȂ�loadMtl�����H
			else
			{
				const char* space = strchr(token, ' ');
				if (space == nullptr)
				{
					space = strchr(token, '\t');
				}

				if (space != nullptr)
				{
					std::ptrdiff_t len = space - token;
					std::string key(token, len);
					std::string value = space + 1;
					material.unknownParameter.insert(std::pair<std::string, std::string>(key, value));
				}
			}
		}

		materialNameMaterialArrayIndexMap.insert(std::pair<std::string, int>(material.name, materials.size()));
		materials.push_back(material);
		return err.str();
	}

	std::string loadObj(const std::string& fileName, std::vector<MeshData>& outMeshArray, std::vector<MaterialData>& outMaterialArray)
	{
		outMeshArray.clear();
		outMaterialArray.clear();

		std::stringstream err;
		std::istringstream fstream(FileUtility::getInstance()->getStringFromFile(fileName));
		if (!fstream) //TODO: istringstrema�͉��^�Ȃ񂾁H
		{
			err << "Cannot open file [" << fileName << "]" << std::endl;
			return err.str();
		}

		const std::string& fullPath = FileUtility::getInstance()->getFullPathForFileName(fileName);
		const std::string& mtlBasePath = fullPath.substr(0, fullPath.find_last_of("\\/") + 1);

		std::vector<Vec3> vertexArray;
		std::vector<Vec2> textureCoordinateArray;
		std::vector<Vec3> normalVertexArray;

		std::vector<std::vector<VertexIndex>> faceGroup;
		std::string name;

		// material
		std::map<std::string, int> materialNameMaterialArrayIndexMap;
		std::map<VertexIndex, unsigned int> vertexIndexPositionsArrayIndexMap;
		int materialIndex = -1;

		MeshData mesh;

		int MAX_CHARS = 8192;
		std::vector<char> buf(MAX_CHARS);

		while (fstream.peek() != -1)
		{
			fstream.getline(&buf[0], MAX_CHARS);

			std::string lineBuf(&buf[0]);

			// Trim newline '\r\n' or '\n'
			size_t size = lineBuf.size();
			if (size > 0)
			{
				if (lineBuf[size - 1] == '\n')
				{
					lineBuf.erase(size - 1);
				}
			}
			size = lineBuf.size();
			if (size > 0)
			{
				if (lineBuf[size - 1] == '\r')
				{
					lineBuf.erase(size - 1);
				}
			}

			// Skip if empty line.
			if (lineBuf.empty())
			{
				continue;
			}

			// Skip leading space.
			const char* token = lineBuf.c_str();
			token += strspn(token, " \t");

			Logger::logAssert(token != nullptr, "�X�y�[�X�����̍s�͖{���͂Ȃ��͂�");

			if (token[0] == '\0')
			{
				continue; // empty line
			}

			if (token[0] == '#')
			{
				continue; // comment line
			}

			// vertex
			if (token[0] == 'v' && isSpace(token[1]))
			{
				token += 2;
				const Vec3& vec = parseVec3(token);
				vertexArray.push_back(vec);
			}
			// normal
			else if (token[0] == 'v' && token[1] == 'n' && isSpace(token[2]))
			{
				token += 3;
				const Vec3& vec = parseVec3(token);
				normalVertexArray.push_back(vec);
			}
			// texture coordinate
			else if (token[0] == 'v' && token[1] == 't' && isSpace(token[2]))
			{
				token += 3;
				const Vec2& vec = parseVec2(token);
				textureCoordinateArray.push_back(vec);
			}
			// face
			else if (token[0] == 'f' && isSpace(token[1]))
			{
				token += 2;
				token += strspn(token, " \t");

				// f�̏ꍇ��n�p�`�|���S���̏ꍇn����ł���̂Ń��[�v�ł��ׂĂƂ�
				std::vector<VertexIndex> face;
				while (!isNewLine(token[0]))
				{
					VertexIndex vi = parseVertexIndex(token, vertexArray.size(), textureCoordinateArray.size(), normalVertexArray.size());
					face.push_back(vi);
					token += strspn(token, " \t\r");
				}

				faceGroup.push_back(face);
			}
			// use mtl
			else if (strncmp(token, "usemtl", 6) == 0 && isSpace(token[6]))
			{
				char nameBuf[SSCANF_BUFFER_SIZE];
				token += 7;

				sscanf_s(token, "%s", nameBuf, _countof(nameBuf));

				bool ret = createMeshDataFromFaceGroup(
					mesh,
					vertexIndexPositionsArrayIndexMap,
					vertexArray,
					normalVertexArray,
					textureCoordinateArray,
					faceGroup,
					materialIndex,
					name,
					true
					);

				if (ret)
				{
					outMeshArray.push_back(mesh);
				}

				mesh = MeshData(); // ���MeshData��ϐ��p�ɍ�蒼��
				faceGroup.clear();

				if (materialNameMaterialArrayIndexMap.find(nameBuf) != materialNameMaterialArrayIndexMap.end())
				{
					materialIndex = materialNameMaterialArrayIndexMap[nameBuf];
				}
				else
				{
					// { error!! material not found }
					materialIndex = -1;
				}
			}
			// load mtl
			else if (strncmp(token, "mtllib", 6) == 0 && isSpace(token[6]))
			{
				char fileNameBuf[SSCANF_BUFFER_SIZE];
				token += 7;

				sscanf_s(token, "%s", fileNameBuf, _countof(fileNameBuf));

				std::string errMtl = loadMtl(fileNameBuf, outMaterialArray, materialNameMaterialArrayIndexMap, mtlBasePath);
				if (!errMtl.empty())
				{
					faceGroup.clear();
					return errMtl;
				}
			}
			// group name
			else if (token[0] == 'g' && isSpace(token[1]))
			{
				// tinyobjloader���Ƃ�����token��i�߂ĂȂ��Bg���g��names[0]�Ƃ��ē����炵��
				token += 2;

				// flush previous face group.
				bool ret = createMeshDataFromFaceGroup(
					mesh,
					vertexIndexPositionsArrayIndexMap,
					vertexArray,
					normalVertexArray,
					textureCoordinateArray,
					faceGroup,
					materialIndex,
					name,
					true
					);
				if (ret)
				{
					outMeshArray.push_back(mesh);
				}

				mesh = MeshData(); // ���MeshData��ϐ��p�ɍ�蒼��
				faceGroup.clear();

				std::vector<std::string> names;
				while (!isNewLine(token[0]))
				{
					std::string str = parseString(token);
					names.push_back(str);
					token += strspn(token, " \t\r");
				}

				Logger::logAssert(names.size() > 0, "obj��g�̃t�H�[�}�b�g����������");
				name = names[0];
			}
			// object name
			else if (token[0] == 'o' && isSpace(token[1]))
			{
				// flush previous face group.
				bool ret = createMeshDataFromFaceGroup(
					mesh,
					vertexIndexPositionsArrayIndexMap,
					vertexArray,
					normalVertexArray,
					textureCoordinateArray,
					faceGroup,
					materialIndex,
					name,
					true
					);
				if (ret)
				{
					outMeshArray.push_back(mesh);
				}

				mesh = MeshData(); // ���MeshData��ϐ��p�ɍ�蒼��
				faceGroup.clear();

				char fileNameBuf[SSCANF_BUFFER_SIZE];
				token += 2;

				sscanf_s(token, "%s", fileNameBuf, _countof(fileNameBuf));
				name = std::string(fileNameBuf);
			}
			// Ignore unknown command.
		}

		bool ret = createMeshDataFromFaceGroup(
			mesh,
			vertexIndexPositionsArrayIndexMap,
			vertexArray,
			normalVertexArray,
			textureCoordinateArray,
			faceGroup,
			materialIndex,
			name,
			true
			);
		if (ret)
		{
			outMeshArray.push_back(mesh);
		}

		// �}�e���A�����ƂɃT�u���b�V���ɕ�������
        char str[20];
		int i = 0;
		std::map<int, std::vector<unsigned short>> subMeshMap;
		for (MeshData& meshData : outMeshArray)
		{
			for (size_t j = 0; j < meshData.numMaterialIndex; j++)
			{
				int materialId = meshData.materialIndices[j];
				size_t index = j * 3;
				subMeshMap[materialId].push_back(meshData.indices[index]);
				subMeshMap[materialId].push_back(meshData.indices[index + 1]);
				subMeshMap[materialId].push_back(meshData.indices[index + 2]);
			}

			for (auto& subMesh : subMeshMap)
			{
				meshData.subMeshIndices.push_back(subMesh.second);
                //mesh->subMeshAABB.push_back(calculateAABB(meshdata->vertex, meshdata->getPerVertexSize(), submesh.second));
                sprintf_s(str, "%d", i++);
				meshData.subMeshIds.push_back(str);
			}
		}

		faceGroup.clear();
		return err.str();
	}

} // namespace ObjLoader

} // namespace mgrrenderer
