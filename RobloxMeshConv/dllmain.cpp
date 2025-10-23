#define _CRT_SECURE_NO_WARNINGS

#include <stdio.h>

#include <string>
#include <sstream>

#include "RobloxMesh.h"

#include <iostream>
#include <cstdint>
#include <cstring>
#include <stdexcept>
#include <vector>

#define ROBLOX_INT uint32_t
#define ROBLOX_SHORT uint16_t
#define ROBLOX_BYTE uint8_t

class ByteReader { //taken from another thing
private:
	const char* buffer;
	size_t size;
	size_t position;

public:
	ByteReader(const char* data, size_t dataSize) : buffer(data), size(dataSize), position(0) {}

	template <typename T>
	T read() {
		if (position + sizeof(T) > size) {
			throw std::out_of_range("attempted to read past the end of the buffer");
		}
		T value;
		std::memcpy(&value, buffer + position, sizeof(T));
		position += sizeof(T);
		return value;
	}

	template <typename T>
	T read(size_t sizee) {
		if (position + sizee > size) {
			throw std::out_of_range("attempted to read past the end of the buffer");
		}
		T value;
		std::memcpy(&value, buffer + position, sizee);
		position += sizee;
		return value;
	}

	std::string readStringSize(size_t length) {
		std::string str;
		size_t limit = position + length;
		for (size_t i = position; i < limit; i++) {
			char c = read<char>();
			str += c;
		}
		return str;
	}

	std::string readString() {
		std::string str;
		while (position < size) {
			char c = read<char>();
			if (c == '\0') {
				break;
			}
			str += c;
		}
		return str;
	}

	void jump(size_t at) {
		if (at > size) {
			throw std::out_of_range("attempted to jump past the end of the buffer");
		}
		position = at;
	}

	void skip(size_t amount) {
		if (position + amount > size) {
			throw std::out_of_range("attempted to skip past the end of the buffer");
		}
		position += amount;
	}

	size_t getPosition() const {
		return position;
	}

	bool hasMoreData() const {
		return position < size;
	}
};

struct v4header {
	ROBLOX_INT vertexCount = 0;
	ROBLOX_INT faceCount = 0;
	ROBLOX_SHORT lodCount = 0;
	ROBLOX_SHORT boneCount = 0;
	ROBLOX_INT nameTableSize = 0;
	ROBLOX_SHORT subsetCount = 0;
};

struct Mesh {
	v4header* headerv4;
	ROBLOX_INT* faces;
	float* vertices;
	float* normals;
	float* uvs;
	ROBLOX_BYTE* tangents;
	bool loaded = false;

	uint32_t lods[2]{0};
};

void debugprint(const char* a) {
#ifdef DEBUG
	printf("[DEBUG] %s\n", a);
#endif
}

void trueassert(bool cond, const char* msg) {
	if (!cond) {
		printf("[RBXMESHCONV] Assertion failed: %s\n", msg);
		throw std::exception(msg); //basic exception
	}
}

template<typename K, typename V>
void swipe(K* array1, V* array2, size_t startOne, size_t startTwo, size_t endTwo) {
	for (size_t i = startTwo; i < endTwo; i++) {
		array1[startOne + (i - startTwo )] = array2[i];
	}
}

std::string convert(char* data, size_t size, char* versione) {
	//mesh version x.xx

	trueassert(size >= 16, "data too short");

	ByteReader reader = ByteReader(data, size);
	std::string versionStr = reader.readStringSize(8);
	if (strncmp(versionStr.c_str(), "version ", 8) == 0) {
		debugprint("Starts with version");
		std::string version = reader.readStringSize(4);
		reader.skip(1);
		v4header* header = new v4header;
		Mesh* mesh = new Mesh;
		trueassert(header != nullptr, "failed to allocate");
		trueassert(mesh != nullptr, "failed to allocate");
		int faceSize = 12;
		int vertexSize = 40;
		int lodSize = 4;
		uint16_t headerSize = 0;
		headerSize = reader.read<uint16_t>(2);

		bool handled = false; //if theres a compatible version
		if (strncmp(version.c_str(), "3.", 2) == 0) {
			handled = true;
			debugprint("Is version 3.00");

			trueassert(headerSize == 16, "version 3.00 headerSize != 16");
			vertexSize = reader.read<ROBLOX_BYTE>(1);
			bool jumpColor = vertexSize == 40; //vertex color check
			faceSize = reader.read<ROBLOX_BYTE>(1); //face size
			lodSize = reader.read<ROBLOX_SHORT>(2); //lod size
			trueassert(vertexSize == 36 || vertexSize == 40, "invalid vertex size, not 36 or 40");
			trueassert(faceSize == 12, "invalid face size, not 12");
			trueassert(lodSize == 4, "invalid lod size, not 4");
			header->lodCount = reader.read<ROBLOX_SHORT>(2);
			header->vertexCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("vc: %d\n", header->vertexCount);
#endif
			header->faceCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("fc: %d\n", header->faceCount);
#endif
			ROBLOX_INT* faces = new ROBLOX_INT[header->faceCount * 3];
			float* vertices = new float[header->vertexCount * 3];
			float* normals = new float[header->vertexCount * 3];
			float* uvs = new float[header->vertexCount * 2];
			//ROBLOX_BYTE* tangents = new ROBLOX_BYTE[header->vertexCount * 4]; tangents not needed

			trueassert(faces != nullptr, "failed to allocate");
			trueassert(vertices != nullptr, "failed to allocate");
			trueassert(normals != nullptr, "failed to allocate");
			trueassert(uvs != nullptr, "failed to allocate");

			float one;
			float two;
			float three;

			for (ROBLOX_INT i = 0; i < header->vertexCount; i++) {
				//vertex
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				vertices[i * 3] = one;
				vertices[i * 3 + 1] = two;
				vertices[i * 3 + 2] = three;
				//normal
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				normals[i * 3] = one;
				normals[i * 3 + 1] = two;
				normals[i * 3 + 2] = three;
				//uv
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				uvs[i * 2] = one;
				uvs[i * 2 + 1] = two;
				if (jumpColor) {
					reader.skip(8); //skipping tangent and vertex color
				}
				else {
					reader.skip(4); //skipping only tangent
				}
			}

			for (ROBLOX_INT i = 0; i < header->faceCount; i++) {
				ROBLOX_INT waa1 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa2 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa3 = reader.read<ROBLOX_INT>(4);
				faces[i * 3] = waa1;
				faces[i * 3 + 1] = waa2;
				faces[i * 3 + 2] = waa3;
			}

			if (header->lodCount <= 2) {
				mesh->lods[0] = 0;
				mesh->lods[1] = header->faceCount;
			}
			else {
				mesh->lods[0] = reader.read<ROBLOX_INT>(4);
				mesh->lods[1] = reader.read<ROBLOX_INT>(4);
			}

			ROBLOX_INT ogtrue = mesh->lods[1] - mesh->lods[0];
			trueassert(ogtrue <= header->faceCount, "highest LOD face count above total face count");
			trueassert(ogtrue >= 0, "highest LOD face count negative");
			ROBLOX_INT trueFaceLength = ogtrue * 3;
			ROBLOX_INT* trueFaces = new ROBLOX_INT[trueFaceLength];
			trueassert(trueFaces != nullptr, "failed to allocate");
			ROBLOX_INT from1 = (mesh->lods[0] * 3);
			ROBLOX_INT to1 = (mesh->lods[1] * 3);
			for (ROBLOX_INT i = from1; i < to1; i++) {
				trueFaces[i] = faces[i];
			}
			ROBLOX_INT maxVertex = 0;
			for (ROBLOX_INT i = 0; i < trueFaceLength; i++) {
				if (trueFaces[i] > maxVertex) {
					maxVertex = trueFaces[i];
				}
			}
			maxVertex += 1;
#ifdef DEBUG
			printf("LODies: %d, %d\n", mesh->lods[0], mesh->lods[1]);
			printf("Is %d lower than %d?\n", maxVertex, header->vertexCount);
#endif
			trueassert(maxVertex <= header->vertexCount, "maxVertex above vertexCount");
			trueassert(maxVertex >= 0, "maxVertex is negative");
			if (maxVertex < header->vertexCount) {
				//fix the vertices and normals and uvs so it doesnt shit itself
				float* trueVertices = new float[maxVertex * 3];
				float* trueNormals = new float[maxVertex * 3];
				float* trueUvs = new float[maxVertex * 2];
				trueassert(trueVertices != nullptr, "failed to allocate");
				trueassert(trueNormals != nullptr, "failed to allocate");
				trueassert(trueUvs != nullptr, "failed to allocate");
				swipe(trueVertices, vertices, 0, 0, maxVertex * 3);
				swipe(trueNormals, normals, 0, 0, maxVertex * 3);
				swipe(trueUvs, uvs, 0, 0, maxVertex * 2);
				mesh->vertices = trueVertices;
				mesh->normals = trueNormals;
				mesh->uvs = trueUvs;
				header->vertexCount = maxVertex;
			}
			else {
				mesh->vertices = vertices;
				mesh->normals = normals;
				mesh->uvs = uvs;
			}
			header->faceCount = ogtrue;
			mesh->headerv4 = header;
			mesh->faces = trueFaces;
			//mesh->tangents = tangents; tangents not needed
			mesh->loaded = true;
		}

		if (strncmp(version.c_str(), "4.", 2) == 0) {
			handled = true;
			reader.skip(2);
			debugprint("Is version 4.00");

			trueassert(headerSize == 24, "version 4.00 headerSize != 24");
			header->vertexCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("vc: %d\n", header->vertexCount);
#endif
			header->faceCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("fc: %d\n", header->faceCount);
#endif
			header->lodCount = reader.read<ROBLOX_SHORT>(2);
			header->boneCount = reader.read<ROBLOX_SHORT>(2);
			header->nameTableSize = reader.read<ROBLOX_INT>(4);
			header->subsetCount = reader.read<ROBLOX_SHORT>(2);
			reader.skip(2);
			ROBLOX_INT* faces = new ROBLOX_INT[header->faceCount * 3];
			float* vertices = new float[header->vertexCount * 3];
			float* normals = new float[header->vertexCount * 3];
			float* uvs = new float[header->vertexCount * 2];
			//ROBLOX_BYTE* tangents = new ROBLOX_BYTE[header->vertexCount * 4]; tangents not needed

			trueassert(faces != nullptr, "failed to allocate");
			trueassert(vertices != nullptr, "failed to allocate");
			trueassert(normals != nullptr, "failed to allocate");
			trueassert(uvs != nullptr, "failed to allocate");

			float one;
			float two;
			float three;

			for (ROBLOX_INT i = 0; i < header->vertexCount; i++) {
				//vertex
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				vertices[i * 3] = one;
				vertices[i * 3 + 1] = two;
				vertices[i * 3 + 2] = three;
				//normal
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				normals[i * 3] = one;
				normals[i * 3 + 1] = two;
				normals[i * 3 + 2] = three;
				//uv
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				uvs[i * 2] = one;
				uvs[i * 2 + 1] = two;
				reader.skip(8); //skipping tangent and vertex color
			}

			//other stuff
			if (header->boneCount > 0) {
				reader.skip((8 * header->vertexCount));
			}
			
			for (ROBLOX_INT i = 0; i < header->faceCount; i++) {
				ROBLOX_INT waa1 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa2 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa3 = reader.read<ROBLOX_INT>(4);
				faces[i * 3] = waa1;
				faces[i * 3 + 1] = waa2;
				faces[i * 3 + 2] = waa3;
			}

			if (header->lodCount <= 2) {
				mesh->lods[0] = 0;
				mesh->lods[1] = header->faceCount;
			}
			else {
				mesh->lods[0] = reader.read<ROBLOX_INT>(4);
				mesh->lods[1] = reader.read<ROBLOX_INT>(4);
			}

			ROBLOX_INT ogtrue = mesh->lods[1] - mesh->lods[0];
			trueassert(ogtrue <= header->faceCount, "highest LOD face count above total face count");
			trueassert(ogtrue >= 0, "highest LOD face count negative");
			ROBLOX_INT trueFaceLength = ogtrue * 3;
			ROBLOX_INT* trueFaces = new ROBLOX_INT[trueFaceLength];
			trueassert(trueFaces != nullptr, "failed to allocate");
			ROBLOX_INT from1 = (mesh->lods[0] * 3);
			ROBLOX_INT to1 = (mesh->lods[1] * 3);
			for (ROBLOX_INT i = from1; i < to1; i++) {
				trueFaces[i] = faces[i];
			}
			ROBLOX_INT maxVertex = 0;
			for (ROBLOX_INT i = 0; i < trueFaceLength; i++) {
				if (trueFaces[i] > maxVertex) {
					maxVertex = trueFaces[i];
				}
			}
			maxVertex += 1;
#ifdef DEBUG
			printf("LODies: %d, %d\n", mesh->lods[0], mesh->lods[1]);
			printf("Is %d lower than %d?\n", maxVertex, header->vertexCount);
#endif
			trueassert(maxVertex <= header->vertexCount, "maxVertex above vertexCount");
			trueassert(maxVertex >= 0, "maxVertex is negative");
			if (maxVertex < header->vertexCount) {
				//fix the vertices and normals and uvs so it doesnt shit itself
				float* trueVertices = new float[maxVertex * 3];
				float* trueNormals = new float[maxVertex * 3];
				float* trueUvs = new float[maxVertex * 2];
				trueassert(trueVertices != nullptr, "failed to allocate");
				trueassert(trueNormals != nullptr, "failed to allocate");
				trueassert(trueUvs != nullptr, "failed to allocate");
				swipe(trueVertices, vertices, 0, 0, maxVertex * 3);
				swipe(trueNormals, normals, 0, 0, maxVertex * 3);
				swipe(trueUvs, uvs, 0, 0, maxVertex * 2);
				mesh->vertices = trueVertices;
				mesh->normals = trueNormals;
				mesh->uvs = trueUvs;
				header->vertexCount = maxVertex;
			}
			else {
				mesh->vertices = vertices;
				mesh->normals = normals;
				mesh->uvs = uvs;
			}
			header->faceCount = ogtrue;
			mesh->headerv4 = header;
			mesh->faces = trueFaces;
			//mesh->tangents = tangents; tangents not needed
			mesh->loaded = true;
		}

		if (strncmp(version.c_str(), "5.", 2) == 0) {
			handled = true;
			reader.skip(2);
			debugprint("Is version 5.00");

			trueassert(headerSize == 32, "version 5.00 headerSize != 32");
			header->vertexCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("vc: %d\n", header->vertexCount);
#endif
			header->faceCount = reader.read<ROBLOX_INT>(4);
#ifdef DEBUG
			printf("fc: %d\n", header->faceCount);
#endif
			header->lodCount = reader.read<ROBLOX_SHORT>(2);
			header->boneCount = reader.read<ROBLOX_SHORT>(2);
			header->nameTableSize = reader.read<ROBLOX_INT>(4);
			header->subsetCount = reader.read<ROBLOX_SHORT>(2);
			reader.skip(10);
			ROBLOX_INT* faces = new ROBLOX_INT[header->faceCount * 3];
			float* vertices = new float[header->vertexCount * 3];
			float* normals = new float[header->vertexCount * 3];
			float* uvs = new float[header->vertexCount * 2];
			//ROBLOX_BYTE* tangents = new ROBLOX_BYTE[header->vertexCount * 4]; tangents not needed

			trueassert(faces != nullptr, "failed to allocate");
			trueassert(vertices != nullptr, "failed to allocate");
			trueassert(normals != nullptr, "failed to allocate");
			trueassert(uvs != nullptr, "failed to allocate");

			float one;
			float two;
			float three;

			for (ROBLOX_INT i = 0; i < header->vertexCount; i++) {
				//vertex
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				vertices[i * 3] = one;
				vertices[i * 3 + 1] = two;
				vertices[i * 3 + 2] = three;
				//normal
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				three = reader.read<float>(4);
				normals[i * 3] = one;
				normals[i * 3 + 1] = two;
				normals[i * 3 + 2] = three;
				//uv
				one = reader.read<float>(4);
				two = reader.read<float>(4);
				uvs[i * 2] = one;
				uvs[i * 2 + 1] = two;
				reader.skip(8); //skipping tangent and vertex color
			}

			//other stuff
			if (header->boneCount > 0) {
				reader.skip((8 * header->vertexCount));
			}

			for (ROBLOX_INT i = 0; i < header->faceCount; i++) {
				ROBLOX_INT waa1 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa2 = reader.read<ROBLOX_INT>(4);
				ROBLOX_INT waa3 = reader.read<ROBLOX_INT>(4);
				faces[i * 3] = waa1;
				faces[i * 3 + 1] = waa2;
				faces[i * 3 + 2] = waa3;
			}

			if (header->lodCount <= 2) {
				mesh->lods[0] = 0;
				mesh->lods[1] = header->faceCount;
			}
			else {
				mesh->lods[0] = reader.read<ROBLOX_INT>(4);
				mesh->lods[1] = reader.read<ROBLOX_INT>(4);
			}

			ROBLOX_INT ogtrue = mesh->lods[1] - mesh->lods[0];
			trueassert(ogtrue <= header->faceCount, "highest LOD face count above total face count");
			trueassert(ogtrue >= 0, "highest LOD face count negative");
			ROBLOX_INT trueFaceLength = ogtrue * 3;
			ROBLOX_INT* trueFaces = new ROBLOX_INT[trueFaceLength];
			trueassert(trueFaces != nullptr, "failed to allocate");
			ROBLOX_INT from1 = (mesh->lods[0] * 3);
			ROBLOX_INT to1 = (mesh->lods[1] * 3);
			for (ROBLOX_INT i = from1; i < to1; i++) {
				trueFaces[i] = faces[i];
			}
			ROBLOX_INT maxVertex = 0;
			for (ROBLOX_INT i = 0; i < trueFaceLength; i++) {
				if (trueFaces[i] > maxVertex) {
					maxVertex = trueFaces[i];
				}
			}
			maxVertex += 1;
#ifdef DEBUG
			printf("LODies: %d, %d\n", mesh->lods[0], mesh->lods[1]);
			printf("Is %d lower than %d?\n", maxVertex, header->vertexCount);
#endif
			trueassert(maxVertex <= header->vertexCount, "maxVertex above vertexCount");
			trueassert(maxVertex >= 0, "maxVertex is negative");
			if (maxVertex < header->vertexCount) {
				//fix the vertices and normals and uvs so it doesnt shit itself
				float* trueVertices = new float[maxVertex * 3];
				float* trueNormals = new float[maxVertex * 3];
				float* trueUvs = new float[maxVertex * 2];
				trueassert(trueVertices != nullptr, "failed to allocate");
				trueassert(trueNormals != nullptr, "failed to allocate");
				trueassert(trueUvs != nullptr, "failed to allocate");
				swipe(trueVertices, vertices, 0, 0, maxVertex * 3);
				swipe(trueNormals, normals, 0, 0, maxVertex * 3);
				swipe(trueUvs, uvs, 0, 0, maxVertex * 2);
				mesh->vertices = trueVertices;
				mesh->normals = trueNormals;
				mesh->uvs = trueUvs;
				header->vertexCount = maxVertex;
			}
			else {
				mesh->vertices = vertices;
				mesh->normals = normals;
				mesh->uvs = uvs;
			}
			header->faceCount = ogtrue;
			mesh->headerv4 = header;
			mesh->faces = trueFaces;
			//mesh->tangents = tangents; tangents not needed
			mesh->loaded = true;
		}

		trueassert(handled, std::string("i cant handle ").append(version).c_str());

		if (mesh->loaded) {
			RobloxMesh roblo;

			debugprint("Mesh loaded");
			if (mesh->headerv4 != nullptr) {
				//V4
				
				for (ROBLOX_INT i = 0; i < mesh->headerv4->vertexCount; i++) {
					RobloxMesh::Vertex v;
					
					v.vx = mesh->vertices[i * 3];
					v.vy = mesh->vertices[i * 3 + 1];
					v.vz = mesh->vertices[i * 3 + 2];
					v.nx = mesh->normals[i * 3];
					v.ny = mesh->normals[i * 3 + 1];
					v.nz = mesh->normals[i * 3 + 2];
					v.tu = mesh->uvs[i * 2];
					v.tv = mesh->uvs[i * 2 + 1];

					roblo.vertices.push_back(v);
				}

				for (ROBLOX_INT i = 0; i < mesh->headerv4->faceCount; i++)
				{
					RobloxMesh::Face meshFace;

					meshFace.a = mesh->faces[i * 3];
					meshFace.b = mesh->faces[i * 3 + 1];
					meshFace.c = mesh->faces[i * 3 + 2];

					roblo.faces.push_back(meshFace);
				}
			}
#ifdef DEBUG
			printf("Vertices is %d\n", mesh->headerv4->vertexCount);
			printf("Faces is %d\n", mesh->headerv4->faceCount);
#endif
			std::ostringstream os;
			if (versione != nullptr) {
				roblo.Write(os, std::string(versione, 4));
			}
			else {
				roblo.Write(os, "2.00");
			}

			std::string output = os.str();
			return output;
		}
		else {
			debugprint("Mesh not loaded");
		}
	}
	else {
		//thats not a mesh
		debugprint("Doesnt start with version");
	}

	return std::string{};
}

extern "C" __declspec(dllexport) char* converte(char* data, size_t size, char* versione, size_t* length) {
	try {
		const std::string& str = convert(data, size, versione);
		if (!str.empty()) {
			char* dat = new char[str.size() + 1];
			trueassert(dat != nullptr, "failed to allocate");
			memcpy(dat, str.c_str(), str.size());
			*length = str.size();
			return dat;
		}
	}
	catch (const std::exception&) {
		//something went wrong
		return nullptr; //just return nullptr
	}

	return nullptr;
}

extern "C" __declspec(dllexport) void freeData(char* data) {
    if(data != nullptr) {
        delete data;
    }
}

//uncomment below if compiling in console
//#define PROGRAM_MODE

#ifdef PROGRAM_MODE
std::string readBinaryFileToString(std::ifstream& file) {
	std::streamsize size = file.tellg();
	file.seekg(0, std::ios::beg);

	std::string buffer(size, '\0');
	if (file.read(&buffer[0], size)) {
		return buffer;
	}
	else {
		return "";
	}
}

int main() {
	std::ifstream i("input.mesh", std::ios::binary | std::ios::ate);

	if (!i.good()) {
		printf("No likey\n");
		exit(1);
	}

	std::string file = readBinaryFileToString(i);
	char* data = new char[file.size()+1];
	memcpy(data, file.c_str(), file.size());
	const std::string& result = convert(data, file.size());
	if (!result.empty()) {
		std::ofstream o("output.mesh", std::ios::binary);
		if (o.good()) {
			debugprint("Writing to file");
			o.write(result.c_str(), result.size());
			o.flush();
			o.close();
		}

		printf("Converted!\n");
		printf("%s\n", result.c_str());
	}
	else {
		printf("Wawa..\n");
		exit(1);
	}
	return 0;
}
#endif