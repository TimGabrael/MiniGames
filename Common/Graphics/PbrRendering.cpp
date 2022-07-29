#include "PbrRendering.h"
#include "Helper.h"
#include "../logging.h"
#include <vector>
#include <stdint.h>
#include "Renderer.h"

#define TINYGLTF_IMPLEMENTATION
#define TINYGLTF_NOEXCEPTION
#define JSON_NOEXCEPTION
#ifdef ANDROID
#define TINYGLTF_ANDROID_LOAD_FROM_ASSETS
#endif
#include "tiny_gltf.h"

bool LoadModel(tinygltf::Model& model, const char* filename)
{
	tinygltf::TinyGLTF loader;
	std::string error; std::string warning;
	bool res = false;
#ifdef _WIN32
	size_t dotIdx = strnlen_s(filename, 250);
#else
	size_t dotIdx = strlen(filename);
#endif
	if (memcmp(filename + dotIdx - 4, ".glb", 4) == 0)
	{
		res = loader.LoadBinaryFromFile(&model, &error, &warning, filename);
	}
	else
	{
		res = loader.LoadASCIIFromFile(&model, &error, &warning, filename);
	}

	if (!warning.empty()) LOG("WARNING WHILE LOAD GLTF FILE: %s warning: %s\n", filename, warning.c_str());
	if (!error.empty()) LOG("FAILED TO LOAD GLTF FILE: %s error: %s\n", filename, error.c_str());


	return res;
}












#define MAX_NUM_JOINTS 128u



struct MaterialUniformData
{
	glm::vec4 baseColorFactor;
	glm::vec4 emissiveFactor;
	glm::vec4 diffuseFactor;
	glm::vec4 specularFactor;
	float workflow;
	int baseColorTextureSet;
	int physicalDescriptorTextureSet;
	int normalTextureSet;
	int occlusionTextureSet;
	int emissiveTextureSet;
	float metallicFactor;
	float roughnessFactor;
	float alphaMask;
	float alphaMaskCutoff;
};



struct Node;
struct BoundingBoxPbr {
	glm::vec3 min;
	glm::vec3 max;
	bool valid = false;
	BoundingBoxPbr() { };
	BoundingBoxPbr(const glm::vec3& min, const glm::vec3& max) :min(min), max(max) { };
	BoundingBoxPbr getAABB(const glm::mat4& m)
	{
		glm::vec3 min = glm::vec3(m[3]);
		glm::vec3 max = min;
		glm::vec3 v0, v1;

		glm::vec3 right = glm::vec3(m[0]);
		v0 = right * this->min.x;
		v1 = right * this->max.x;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 up = glm::vec3(m[1]);
		v0 = up * this->min.y;
		v1 = up * this->max.y;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		glm::vec3 back = glm::vec3(m[2]);
		v0 = back * this->min.z;
		v1 = back * this->max.z;
		min += glm::min(v0, v1);
		max += glm::max(v0, v1);

		return BoundingBoxPbr(min, max);
	}
};

struct Vertex {
	glm::vec3 pos;
	glm::vec3 normal;
	glm::vec2 uv0;
	glm::vec2 uv1;
	glm::vec4 joint0;
	glm::vec4 weight0;
};
struct LoaderInfo {
	uint32_t* indexBuffer;
	Vertex* vertexBuffer;
	size_t indexPos = 0;
	size_t vertexPos = 0;
};
struct Skin {
	std::string name;
	Node* skeletonRoot = nullptr;
	std::vector<glm::mat4> inverseBindMatrices;
	std::vector<Node*> joints;
};
struct MaterialPbr
{
	enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
	AlphaMode alphaMode = ALPHAMODE_OPAQUE;
	float alphaCutoff = 1.0f;
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	glm::vec4 baseColorFactor = glm::vec4(1.0f);
	glm::vec4 emissiveFactor = glm::vec4(1.0f);
	GLuint uniformBuffer = 0;
	GLuint baseColorTexture = 0;
	GLuint metallicRoughnessTexture = 0;
	GLuint normalTexture = 0;
	GLuint occlusionTexture = 0;
	GLuint emissiveTexture = 0;
	struct TexCoorSets {
		uint8_t baseColor = 0; 
		uint8_t metallicRoughness = 0;
		uint8_t specularGlossiness = 0;
		uint8_t normal = 0;
		uint8_t occlusion = 0;
		uint8_t emissive = 0;
	}texCoordSets;
	struct Extension {
		GLuint specularGlossinessTexture = 0;
		GLuint diffuseTexture = 0;
		glm::vec4 diffuseFactor = glm::vec4(1.0f);
		glm::vec3 specularFactor = { 0.0f, 0.0f, 0.0f };
	}extension;
	struct PbrWorkflows {
		bool metallicRoughness = true;
		bool specularGlossiness = false;
	}pbrWorkflows;

};
struct Primitive {
	Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, MaterialPbr& material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
		hasIndices = indexCount > 0;
	}
	void SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		bb.min = min; bb.max = max; bb.valid = true;
	}
	uint32_t firstIndex; uint32_t indexCount; uint32_t vertexCount;
	MaterialPbr& material;
	bool hasIndices;
	BoundingBoxPbr bb;
};

struct MeshPbr {
	MeshPbr(const glm::mat4& matrix) {
		uniformBlock.matrix = matrix;
		glGenBuffers(1, &uniformBuffer.handle);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer.handle);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(uniformBlock), &uniformBlock, GL_DYNAMIC_DRAW);

	}
	void SetBoundingBox(const glm::vec3& min, const glm::vec3& max) {
		bb.min = min; bb.max = max; bb.valid = true;
	}
	~MeshPbr() {
		glDeleteBuffers(1, &uniformBuffer.handle);
		for (Primitive* p : primitives)
			delete p;
	}
	std::vector<Primitive*> primitives;
	BoundingBoxPbr bb;
	BoundingBoxPbr aabb;
	struct UniformBuffer {
		GLuint handle;
	}uniformBuffer;
	struct UniformBlock {
		glm::mat4 matrix;
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
		float jointcount = 0;
	}uniformBlock;

};
struct Node {
	Node* parent = nullptr;
	uint32_t index;
	std::vector<Node*> children;
	glm::mat4 matrix;
	std::string name;
	MeshPbr* mesh = nullptr;
	Skin* skin = nullptr;
	int32_t skinIndex = -1;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};
	BoundingBoxPbr bvh;
	BoundingBoxPbr aabb;
	glm::mat4 localMatrix()
	{
		return glm::translate(glm::mat4(1.0f), translation) * glm::mat4(rotation) * glm::scale(glm::mat4(1.0f), scale) * matrix;
	}
	glm::mat4 getMatrix()
	{
		glm::mat4 m = localMatrix();
		Node* p = parent;
		while (p) {
			m = p->localMatrix() * m;
			p = p->parent;
		}
		return m;
	}
	void update()
	{
		if (mesh) {
			glBindBuffer(GL_UNIFORM_BUFFER, mesh->uniformBuffer.handle);
			void* mapped = glMapBufferRange(GL_UNIFORM_BUFFER, 0, sizeof(MeshPbr::UniformBlock), GL_MAP_WRITE_BIT| GL_MAP_INVALIDATE_BUFFER_BIT);
			if (mapped)
			{
				glm::mat4 rot = glm::mat4(1.0f);
				glm::mat4 m = rot * getMatrix();
				if (skin) {
					mesh->uniformBlock.matrix = m;
					// Update join matrices
					glm::mat4 inverseTransform = glm::inverse(m);
					size_t numJoints = std::min((uint32_t)skin->joints.size(), MAX_NUM_JOINTS);
					for (size_t i = 0; i < numJoints; i++) {
						Node* jointNode = skin->joints[i];
						glm::mat4 jointMat = jointNode->getMatrix() * skin->inverseBindMatrices[i];
						jointMat = inverseTransform * jointMat;
						mesh->uniformBlock.jointMatrix[i] = jointMat;
					}
					mesh->uniformBlock.jointcount = (float)numJoints;
					memcpy(mapped, &mesh->uniformBlock, sizeof(MeshPbr::UniformBlock));
				}
				else {
					memset(mapped, 0, sizeof(MeshPbr::UniformBlock));
					memcpy(mapped, &m, sizeof(glm::mat4));
				}
				auto result = glUnmapBuffer(GL_UNIFORM_BUFFER);
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}
	~Node()
	{
		if (mesh)delete mesh;
		mesh = nullptr;
		for (auto& child : children) {
			delete child;
			child = nullptr;
		}
	}
};

struct AnimationChannel {
	enum PathType { TRANSLATION, ROTATION, SCALE };
	PathType path;
	Node* node;
	uint32_t samplerIndex;
};

struct AnimationSampler {
	enum InterpolationType { LINEAR, STEP, CUBICSPLINE };
	InterpolationType interpolation;
	std::vector<float> inputs;
	std::vector<glm::vec4> outputsVec4;
};
struct Animation
{
	std::string name;
	std::vector<AnimationSampler> samplers;
	std::vector<AnimationChannel> channels;
	float start = std::numeric_limits<float>::max();
	float end = std::numeric_limits<float>::min();
};







struct InternalPBR
{
	GLuint vao;
	GLuint vertexBuffer;
	struct Indices {
		GLuint indexBuffer;
		int count;
	}indices;
	glm::mat4 aabb;
	std::vector<GLuint> textures;
	std::vector<MaterialPbr> materials;
	std::vector<Node*> nodes;
	std::vector<Node*> linearNodes;
	std::vector<Animation> animations;
	std::vector<Skin*> skins;

	struct Dimensions {
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
	} dimensions;
	float animationTime = 0.0f;

};
static constexpr size_t internalPBRSize = sizeof(InternalPBR);




void LoadTextures(const tinygltf::Model& m, InternalPBR& pbr)
{
	for (auto& tex : m.textures)
	{
		const tinygltf::Image& img = m.images[tex.source];

		unsigned char* buffer = nullptr;
		size_t bufferSize = 0;
		bool deleteBuffer = false;

		if (img.component == 3)
		{
			bufferSize = img.width * img.height * 4;
			buffer = new unsigned char[bufferSize];
			unsigned char* rgba = buffer;
			const unsigned char* rgb = &img.image.at(0);
			for (int32_t i = 0; i < img.width * img.height; i++)
			{
				for (int32_t j = 0; j < 3; j++)
				{
					buffer[j] = rgb[i];
				}
				rgba += 4;
				rgb += 3;
			}
			deleteBuffer = true;
		}
		else
		{
			buffer = (unsigned char*)&img.image.at(0);
			bufferSize = img.image.size();
		}
		int mipLevels = (uint32_t)(floor(log2(std::max(img.width, img.height))) + 1.0);



		GLuint curTexture;
		glGenTextures(1, &curTexture);
		glBindTexture(GL_TEXTURE_2D, curTexture);
#ifdef __glad_h_	// not using opengles
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16);
#endif
		if (tex.sampler != -1)
		{
			const tinygltf::Sampler& smpl = m.samplers.at(tex.sampler);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, smpl.wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, smpl.wrapT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.image.data());
		glGenerateMipmap(GL_TEXTURE_2D);	// maybe i shouldn't, but for now it stays



		if (deleteBuffer)
		{
			delete buffer;
		}

		pbr.textures.push_back(curTexture);
	}
}

void LoadMaterials(tinygltf::Model& m, InternalPBR& pbr)
{
	for (auto& mat : m.materials)
	{
		MaterialPbr material{};
		if (mat.values.find("baseColorTexture") != mat.values.end())
		{
			material.baseColorTexture = pbr.textures[mat.values["baseColorTexture"].TextureIndex()];
			material.texCoordSets.baseColor = mat.values["baseColorTexture"].TextureTexCoord();
		}
		if (mat.values.find("metallicRoughnessTexture") != mat.values.end()) {
			material.metallicRoughnessTexture = pbr.textures[mat.values["metallicRoughnessTexture"].TextureIndex()];
			material.texCoordSets.metallicRoughness = mat.values["metallicRoughnessTexture"].TextureTexCoord();
		}
		if (mat.values.find("roughnessFactor") != mat.values.end()) {
			material.roughnessFactor = static_cast<float>(mat.values["roughnessFactor"].Factor());
		}
		if (mat.values.find("metallicFactor") != mat.values.end()) {
			material.metallicFactor = static_cast<float>(mat.values["metallicFactor"].Factor());
		}
		if (mat.values.find("baseColorFactor") != mat.values.end()) {
			material.baseColorFactor = glm::make_vec4(mat.values["baseColorFactor"].ColorFactor().data());
		}
		if (mat.additionalValues.find("normalTexture") != mat.additionalValues.end()) {
			material.normalTexture = pbr.textures[mat.additionalValues["normalTexture"].TextureIndex()];
			material.texCoordSets.normal = mat.additionalValues["normalTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("emissiveTexture") != mat.additionalValues.end()) {
			material.emissiveTexture = pbr.textures[mat.additionalValues["emissiveTexture"].TextureIndex()];
			material.texCoordSets.emissive = mat.additionalValues["emissiveTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("occlusionTexture") != mat.additionalValues.end()) {
			material.occlusionTexture = pbr.textures[mat.additionalValues["occlusionTexture"].TextureIndex()];
			material.texCoordSets.occlusion = mat.additionalValues["occlusionTexture"].TextureTexCoord();
		}
		if (mat.additionalValues.find("alphaMode") != mat.additionalValues.end()) {
			tinygltf::Parameter param = mat.additionalValues["alphaMode"];
			if (param.string_value == "BLEND") {
				material.alphaMode = MaterialPbr::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material.alphaCutoff = 0.5f;
				material.alphaMode = MaterialPbr::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
			material.emissiveFactor = glm::vec4(glm::make_vec3(mat.additionalValues["emissiveFactor"].ColorFactor().data()), 1.0);
		}

		if (mat.extensions.find("KHR_materials_pbrSpecularGlossiness") != mat.extensions.end()) {
			auto ext = mat.extensions.find("KHR_materials_pbrSpecularGlossiness");
			if (ext->second.Has("specularGlossinessTexture")) {
				auto index = ext->second.Get("specularGlossinessTexture").Get("index");
				material.extension.specularGlossinessTexture = pbr.textures[index.Get<int>()];
				auto texCoordSet = ext->second.Get("specularGlossinessTexture").Get("texCoord");
				material.texCoordSets.specularGlossiness = texCoordSet.Get<int>();
				material.pbrWorkflows.specularGlossiness = true;
			}
			if (ext->second.Has("diffuseTexture")) {
				auto index = ext->second.Get("diffuseTexture").Get("index");
				material.extension.diffuseTexture = pbr.textures[index.Get<int>()];
			}
			if (ext->second.Has("diffuseFactor")) {
				auto factor = ext->second.Get("diffuseFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
					auto val = factor.Get(i);
					material.extension.diffuseFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
			if (ext->second.Has("specularFactor")) {
				auto factor = ext->second.Get("specularFactor");
				for (uint32_t i = 0; i < factor.ArrayLen(); i++) {
					auto val = factor.Get(i);
					material.extension.specularFactor[i] = val.IsNumber() ? (float)val.Get<double>() : (float)val.Get<int>();
				}
			}
		}

		glGenBuffers(1, &material.uniformBuffer);
		glBindBuffer(GL_UNIFORM_BUFFER, material.uniformBuffer);
		MaterialUniformData data;
		memset(&data, 0, sizeof(MaterialUniformData));
		data.baseColorFactor = material.baseColorFactor;
		data.emissiveFactor = material.emissiveFactor;
		data.diffuseFactor = material.extension.diffuseFactor;
		data.specularFactor = glm::vec4(material.extension.specularFactor, 0.0f);
		data.baseColorTextureSet = material.baseColorTexture ? material.texCoordSets.baseColor : -1;
		data.normalTextureSet = material.normalTexture ? material.texCoordSets.normal : -1;
		data.occlusionTextureSet = material.occlusionTexture ? material.texCoordSets.occlusion : -1;
		data.emissiveTextureSet = material.emissiveTexture ? material.texCoordSets.emissive : -1;
		data.metallicFactor = material.metallicFactor;
		data.roughnessFactor = material.roughnessFactor;
		data.alphaMask = (float)(material.alphaMode == MaterialPbr::ALPHAMODE_MASK);
		data.alphaMaskCutoff = material.alphaCutoff;

		if (material.pbrWorkflows.metallicRoughness)
		{
			data.workflow = 0.0f;
			data.physicalDescriptorTextureSet = material.pbrWorkflows.metallicRoughness ? material.texCoordSets.metallicRoughness :  -1;
		}
		if (material.pbrWorkflows.specularGlossiness)
		{
			data.workflow = 1.0f;
			data.physicalDescriptorTextureSet = material.extension.specularGlossinessTexture ? material.texCoordSets.specularGlossiness : -1;
			data.baseColorTextureSet = material.extension.diffuseTexture ? material.texCoordSets.baseColor : -1;
			data.specularFactor = glm::vec4(material.extension.specularFactor, 1.0f);
		}
		glBufferData(GL_UNIFORM_BUFFER, sizeof(MaterialUniformData), &data, GL_STATIC_DRAW);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
		

		pbr.materials.push_back(material);
	}
	pbr.materials.push_back(MaterialPbr());
}

void GetNodeProps(const tinygltf::Node& node, const tinygltf::Model& model, size_t& vertexCount, size_t& indexCount)
{
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++)
		{
			GetNodeProps(model.nodes[node.children[i]], model, vertexCount, indexCount);
		}
	}
	if (node.mesh > -1) {
		const tinygltf::Mesh& mesh = model.meshes[node.mesh];
		for (size_t i = 0; i < mesh.primitives.size(); i++)
		{
			const auto& primitive = mesh.primitives.at(i);
			vertexCount += model.accessors[primitive.attributes.find("POSITION")->second].count;
			if (primitive.indices > -1) {
				indexCount += model.accessors[primitive.indices].count;
			}
		}
	}
}

void LoadNode(InternalPBR& pbr, Node* parent, const tinygltf::Node& node, uint32_t nodeIndex, const tinygltf::Model& model, LoaderInfo& loaderInfo, float globalScale)
{
	Node* newNode = new Node{};
	newNode->index = nodeIndex;
	newNode->parent = parent;
	newNode->skinIndex = node.skin;
	newNode->matrix = glm::mat4(1.0f);

	glm::vec3 translation = glm::vec3(0.0f);
	if (node.translation.size() == 3) {
		translation = glm::make_vec3(node.translation.data());
		newNode->translation = translation * globalScale;
	}
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		const double* rot = node.rotation.data();
		glm::quat q = glm::quat(rot[3], rot[0], rot[1], rot[2]);
		newNode->rotation = glm::mat4(q);
	}
	glm::vec3 scale = glm::vec3(1.0f);
	if (node.scale.size() == 3) {
		scale = glm::make_vec3(node.scale.data());
		newNode->scale = scale;
	}
	if (node.matrix.size() == 16) {
		newNode->matrix = glm::make_mat4x4(node.matrix.data());
	};

	// Node with children
	if (node.children.size() > 0) {
		for (size_t i = 0; i < node.children.size(); i++) {
			LoadNode(pbr, newNode, model.nodes[node.children[i]], node.children[i], model, loaderInfo, globalScale);
		}
	}

	// Node contains mesh data
	if (node.mesh > -1) {
		const tinygltf::Mesh mesh = model.meshes[node.mesh];
		MeshPbr* newMesh = new MeshPbr(newNode->matrix);
		for (size_t j = 0; j < mesh.primitives.size(); j++) {
			const tinygltf::Primitive& primitive = mesh.primitives[j];
			uint32_t vertexStart = loaderInfo.vertexPos;
			uint32_t indexStart = loaderInfo.indexPos;
			uint32_t indexCount = 0;
			uint32_t vertexCount = 0;
			glm::vec3 posMin{};
			glm::vec3 posMax{};
			bool hasSkin = false;
			bool hasIndices = primitive.indices > -1;
			// Vertices
			{
				const float* bufferPos = nullptr;
				const float* bufferNormals = nullptr;
				const float* bufferTexCoordSet0 = nullptr;
				const float* bufferTexCoordSet1 = nullptr;
				const void* bufferJoints = nullptr;
				const float* bufferWeights = nullptr;

				int posByteStride;
				int normByteStride;
				int uv0ByteStride;
				int uv1ByteStride;
				int jointByteStride;
				int weightByteStride;

				int jointComponentType;

				// Position attribute is required
				assert(primitive.attributes.find("POSITION") != primitive.attributes.end());

				const tinygltf::Accessor& posAccessor = model.accessors[primitive.attributes.find("POSITION")->second];
				const tinygltf::BufferView& posView = model.bufferViews[posAccessor.bufferView];
				bufferPos = reinterpret_cast<const float*>(&(model.buffers[posView.buffer].data[posAccessor.byteOffset + posView.byteOffset]));
				posMin = glm::vec3(posAccessor.minValues[0], posAccessor.minValues[1], posAccessor.minValues[2]);
				posMax = glm::vec3(posAccessor.maxValues[0], posAccessor.maxValues[1], posAccessor.maxValues[2]);
				vertexCount = static_cast<uint32_t>(posAccessor.count);
				posByteStride = posAccessor.ByteStride(posView) ? (posAccessor.ByteStride(posView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);

				if (primitive.attributes.find("NORMAL") != primitive.attributes.end()) {
					const tinygltf::Accessor& normAccessor = model.accessors[primitive.attributes.find("NORMAL")->second];
					const tinygltf::BufferView& normView = model.bufferViews[normAccessor.bufferView];
					bufferNormals = reinterpret_cast<const float*>(&(model.buffers[normView.buffer].data[normAccessor.byteOffset + normView.byteOffset]));
					normByteStride = normAccessor.ByteStride(normView) ? (normAccessor.ByteStride(normView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC3);
				}

				if (primitive.attributes.find("TEXCOORD_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_0")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet0 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv0ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}
				if (primitive.attributes.find("TEXCOORD_1") != primitive.attributes.end()) {
					const tinygltf::Accessor& uvAccessor = model.accessors[primitive.attributes.find("TEXCOORD_1")->second];
					const tinygltf::BufferView& uvView = model.bufferViews[uvAccessor.bufferView];
					bufferTexCoordSet1 = reinterpret_cast<const float*>(&(model.buffers[uvView.buffer].data[uvAccessor.byteOffset + uvView.byteOffset]));
					uv1ByteStride = uvAccessor.ByteStride(uvView) ? (uvAccessor.ByteStride(uvView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC2);
				}

				// Skinning
				// Joints
				if (primitive.attributes.find("JOINTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& jointAccessor = model.accessors[primitive.attributes.find("JOINTS_0")->second];
					const tinygltf::BufferView& jointView = model.bufferViews[jointAccessor.bufferView];
					bufferJoints = &(model.buffers[jointView.buffer].data[jointAccessor.byteOffset + jointView.byteOffset]);
					jointComponentType = jointAccessor.componentType;
					jointByteStride = jointAccessor.ByteStride(jointView) ? (jointAccessor.ByteStride(jointView) / tinygltf::GetComponentSizeInBytes(jointComponentType)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				if (primitive.attributes.find("WEIGHTS_0") != primitive.attributes.end()) {
					const tinygltf::Accessor& weightAccessor = model.accessors[primitive.attributes.find("WEIGHTS_0")->second];
					const tinygltf::BufferView& weightView = model.bufferViews[weightAccessor.bufferView];
					bufferWeights = reinterpret_cast<const float*>(&(model.buffers[weightView.buffer].data[weightAccessor.byteOffset + weightView.byteOffset]));
					weightByteStride = weightAccessor.ByteStride(weightView) ? (weightAccessor.ByteStride(weightView) / sizeof(float)) : tinygltf::GetNumComponentsInType(TINYGLTF_TYPE_VEC4);
				}

				hasSkin = (bufferJoints && bufferWeights);

				for (size_t v = 0; v < posAccessor.count; v++) {
					Vertex& vert = loaderInfo.vertexBuffer[loaderInfo.vertexPos];
					vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]) * globalScale, 1.0f);
					vert.normal = glm::normalize(glm::vec3(bufferNormals ? glm::make_vec3(&bufferNormals[v * normByteStride]) : glm::vec3(0.0f)));
					vert.uv0 = bufferTexCoordSet0 ? glm::make_vec2(&bufferTexCoordSet0[v * uv0ByteStride]) : glm::vec3(0.0f);
					vert.uv1 = bufferTexCoordSet1 ? glm::make_vec2(&bufferTexCoordSet1[v * uv1ByteStride]) : glm::vec3(0.0f);

					if (hasSkin)
					{
						switch (jointComponentType) {
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_SHORT: {
							const uint16_t* buf = static_cast<const uint16_t*>(bufferJoints);
							vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						case TINYGLTF_COMPONENT_TYPE_UNSIGNED_BYTE: {
							const uint8_t* buf = static_cast<const uint8_t*>(bufferJoints);
							vert.joint0 = glm::vec4(glm::make_vec4(&buf[v * jointByteStride]));
							break;
						}
						default:
							// Not supported by spec
							LOG("Joint component type %d not supported\n", jointComponentType);
							break;
						}
					}
					else {
						vert.joint0 = glm::vec4(0.0f);
					}
					vert.weight0 = hasSkin ? glm::make_vec4(&bufferWeights[v * weightByteStride]) : glm::vec4(0.0f);
					// Fix for all zero weights
					if (glm::length(vert.weight0) == 0.0f) {
						vert.weight0 = glm::vec4(1.0f, 0.0f, 0.0f, 0.0f);
					}
					loaderInfo.vertexPos++;
				}
			}
			// Indices
			if (hasIndices)
			{
				const tinygltf::Accessor& accessor = model.accessors[primitive.indices > -1 ? primitive.indices : 0];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				indexCount = static_cast<uint32_t>(accessor.count);
				const void* dataPtr = &(buffer.data[accessor.byteOffset + bufferView.byteOffset]);

				switch (accessor.componentType) {
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_INT: {
					const uint32_t* buf = static_cast<const uint32_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_SHORT: {
					const uint16_t* buf = static_cast<const uint16_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				case TINYGLTF_PARAMETER_TYPE_UNSIGNED_BYTE: {
					const uint8_t* buf = static_cast<const uint8_t*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						loaderInfo.indexBuffer[loaderInfo.indexPos] = buf[index] + vertexStart;
						loaderInfo.indexPos++;
					}
					break;
				}
				default:
					LOG("Index component type %d not supported!\n", accessor.componentType);
					return;
				}
			}
			Primitive* newPrimitive = new Primitive(indexStart, indexCount, vertexCount, primitive.material > -1 ? pbr.materials[primitive.material] : pbr.materials.back());
			newPrimitive->SetBoundingBox(posMin, posMax);
			newMesh->primitives.push_back(newPrimitive);
		}
		// Mesh BB from BBs of primitives
		for (auto p : newMesh->primitives) {
			if (p->bb.valid && !newMesh->bb.valid) {
				newMesh->bb = p->bb;
				newMesh->bb.valid = true;
			}
			newMesh->bb.min = glm::min(newMesh->bb.min, p->bb.min);
			newMesh->bb.max = glm::max(newMesh->bb.max, p->bb.max);
		}
		newNode->mesh = newMesh;
	}
	if (parent) {
		parent->children.push_back(newNode);
	}
	else {
		pbr.nodes.push_back(newNode);
	}
	pbr.linearNodes.push_back(newNode);
}

Node* findNode(InternalPBR& pbr, Node* parent, uint32_t index) {
	Node* nodeFound = nullptr;
	if (parent->index == index) {
		return parent;
	}
	for (auto& child : parent->children) {
		nodeFound = findNode(pbr, child, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}
Node* NodeFromIndex(InternalPBR& pbr, uint32_t index) {
	Node* nodeFound = nullptr;
	for (auto& node : pbr.nodes) {
		nodeFound = findNode(pbr, node, index);
		if (nodeFound) {
			break;
		}
	}
	return nodeFound;
}

void LoadAnimations(InternalPBR& pbr, tinygltf::Model& model)
{
	for (tinygltf::Animation& anim : model.animations)
	{
		Animation animation{};
		animation.name = anim.name;
		if (anim.name.empty()) {
			animation.name = std::to_string(pbr.animations.size());
		}

		// Samplers
		for (auto& samp : anim.samplers) {
			AnimationSampler sampler{};

			if (samp.interpolation == "LINEAR") {
				sampler.interpolation = AnimationSampler::InterpolationType::LINEAR;
			}
			if (samp.interpolation == "STEP") {
				sampler.interpolation = AnimationSampler::InterpolationType::STEP;
			}
			if (samp.interpolation == "CUBICSPLINE") {
				sampler.interpolation = AnimationSampler::InterpolationType::CUBICSPLINE;
			}

			// Read sampler input time values
			{
				const tinygltf::Accessor& accessor = model.accessors[samp.input];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];
				const float* buf = static_cast<const float*>(dataPtr);
				for (size_t index = 0; index < accessor.count; index++) {
					sampler.inputs.push_back(buf[index]);
				}

				for (auto input : sampler.inputs) {
					if (input < animation.start) {
						animation.start = input;
					};
					if (input > animation.end) {
						animation.end = input;
					}
				}
			}

			// Read sampler output T/R/S values 
			{
				const tinygltf::Accessor& accessor = model.accessors[samp.output];
				const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
				const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];

				assert(accessor.componentType == TINYGLTF_COMPONENT_TYPE_FLOAT);

				const void* dataPtr = &buffer.data[accessor.byteOffset + bufferView.byteOffset];

				switch (accessor.type) {
				case TINYGLTF_TYPE_VEC3: {
					const glm::vec3* buf = static_cast<const glm::vec3*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						sampler.outputsVec4.push_back(glm::vec4(buf[index], 0.0f));
					}
					break;
				}
				case TINYGLTF_TYPE_VEC4: {
					const glm::vec4* buf = static_cast<const glm::vec4*>(dataPtr);
					for (size_t index = 0; index < accessor.count; index++) {
						sampler.outputsVec4.push_back(buf[index]);
					}
					break;
				}
				default: {
					LOG("unknown type\n");
					break;
				}
				}
			}

			animation.samplers.push_back(sampler);
		}

		// Channels
		for (auto& source : anim.channels) {
			AnimationChannel channel{};

			if (source.target_path == "rotation") {
				channel.path = AnimationChannel::PathType::ROTATION;
			}
			if (source.target_path == "translation") {
				channel.path = AnimationChannel::PathType::TRANSLATION;
			}
			if (source.target_path == "scale") {
				channel.path = AnimationChannel::PathType::SCALE;
			}
			if (source.target_path == "weights") {
				LOG("weights not yet supported, skipping channel\n");
				continue;
			}
			channel.samplerIndex = source.sampler;
			channel.node = NodeFromIndex(pbr, source.target_node);
			if (!channel.node) {
				continue;
			}

			animation.channels.push_back(channel);
		}

		pbr.animations.push_back(animation);
	
	}
}

void LoadSkins(InternalPBR& pbr, tinygltf::Model& model) 
{
	for (tinygltf::Skin& source : model.skins) {
		Skin* newSkin = new Skin{};
		newSkin->name = source.name;

		if (source.skeleton > -1) {
			newSkin->skeletonRoot = NodeFromIndex(pbr, source.skeleton);
		}

		for (int jointIndex : source.joints) {
			Node* node = NodeFromIndex(pbr, jointIndex);
			if (node) {
				newSkin->joints.push_back(NodeFromIndex(pbr, jointIndex));
			}
		}

		if (source.inverseBindMatrices > -1) {
			const tinygltf::Accessor& accessor = model.accessors[source.inverseBindMatrices];
			const tinygltf::BufferView& bufferView = model.bufferViews[accessor.bufferView];
			const tinygltf::Buffer& buffer = model.buffers[bufferView.buffer];
			newSkin->inverseBindMatrices.resize(accessor.count);
			memcpy(newSkin->inverseBindMatrices.data(), &buffer.data[accessor.byteOffset + bufferView.byteOffset], accessor.count * sizeof(glm::mat4));
		}

		pbr.skins.push_back(newSkin);
	}
}

void CalculateBoundingBox(InternalPBR& pbr, Node* node, Node* parent) {
	BoundingBoxPbr parentBvh = parent ? parent->bvh : BoundingBoxPbr(pbr.dimensions.min, pbr.dimensions.max);

	if (node->mesh) {
		if (node->mesh->bb.valid) {
			node->aabb = node->mesh->bb.getAABB(node->getMatrix());
			if (node->children.size() == 0) {
				node->bvh.min = node->aabb.min;
				node->bvh.max = node->aabb.max;
				node->bvh.valid = true;
			}
		}
	}

	parentBvh.min = glm::min(parentBvh.min, node->bvh.min);
	parentBvh.max = glm::min(parentBvh.max, node->bvh.max);

	for (auto& child : node->children) {
		CalculateBoundingBox(pbr, child, node);
	}
}
void GetSceneDimensions(InternalPBR& pbr)
{
	for (auto node : pbr.linearNodes) {
		CalculateBoundingBox(pbr, node, nullptr);
	}

	pbr.dimensions.min = glm::vec3(FLT_MAX);
	pbr.dimensions.max = glm::vec3(-FLT_MAX);

	for (auto node : pbr.linearNodes) {
		if (node->bvh.valid) {
			pbr.dimensions.min = glm::min(pbr.dimensions.min, node->bvh.min);
			pbr.dimensions.max = glm::max(pbr.dimensions.max, node->bvh.max);
		}
	}

	// Calculate scene aabb
	pbr.aabb = glm::scale(glm::mat4(1.0f), glm::vec3(pbr.dimensions.max[0] - pbr.dimensions.min[0], pbr.dimensions.max[1] - pbr.dimensions.min[1], pbr.dimensions.max[2] - pbr.dimensions.min[2]));
	pbr.aabb[3][0] = pbr.dimensions.min[0];
	pbr.aabb[3][1] = pbr.dimensions.min[1];
	pbr.aabb[3][2] = pbr.dimensions.min[2];
}
void* CreateInternalPBRFromFile(const char* filename, float scale)
{
	tinygltf::Model model;
	if (!LoadModel(model, filename)) {
		return nullptr;
	}
	InternalPBR* resultPbr = new InternalPBR;
	InternalPBR& pbr = *resultPbr;
	
	LoaderInfo loaderInfo{};
	size_t vertexCount = 0;
	size_t indexCount = 0;



	LoadTextures(model, pbr);
	LoadMaterials(model, pbr);
	const tinygltf::Scene& scene = model.scenes[model.defaultScene > -1 ? model.defaultScene : 0];
	for (size_t i = 0; i < scene.nodes.size(); i++)
	{
		GetNodeProps(model.nodes[scene.nodes[i]], model, vertexCount, indexCount);
	}
	loaderInfo.vertexBuffer = new Vertex[vertexCount];
	loaderInfo.indexBuffer = new uint32_t[indexCount];
	for (size_t i = 0; i < scene.nodes.size(); i++) {
		const tinygltf::Node& node = model.nodes[scene.nodes[i]];
		LoadNode(pbr, nullptr, node, scene.nodes[i], model, loaderInfo, scale);
	}
	if (model.animations.size() > 0) {
		LoadAnimations(pbr, model);
	}
	LoadSkins(pbr, model);

	for (auto& node : pbr.linearNodes)
	{
		if (node->skinIndex > -1) {
			node->skin = pbr.skins[node->skinIndex];
		}
		if (node->mesh) {
			node->update();
		}
	}

	size_t vertexBufferSize = vertexCount * sizeof(Vertex);
	size_t indexBufferSize = indexCount * sizeof(uint32_t);
	pbr.indices.count = indexCount;


	glGenVertexArrays(1, &pbr.vao);
	glBindVertexArray(pbr.vao);

	glGenBuffers(1, &pbr.vertexBuffer);
	glGenBuffers(1, &pbr.indices.indexBuffer);

	glBindBuffer(GL_ARRAY_BUFFER, pbr.vertexBuffer);
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, loaderInfo.vertexBuffer, GL_STATIC_DRAW);

	
	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);
	
	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(6 * sizeof(float)));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(8 * sizeof(float)));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(10 * sizeof(float)));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, sizeof(Vertex), (const void*)(14 * sizeof(float)));


	glBindVertexArray(0);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, pbr.indices.indexBuffer);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, loaderInfo.indexBuffer, GL_STATIC_DRAW);


	delete[] loaderInfo.vertexBuffer;
	delete[] loaderInfo.indexBuffer;

	return resultPbr;
}
void CleanUpInternal(void* internalObj)
{
	InternalPBR* realObj = (InternalPBR*)internalObj;

	glDeleteVertexArrays(1, &realObj->vao);
	glDeleteBuffers(1, &realObj->vertexBuffer);
	glDeleteBuffers(1, &realObj->indices.indexBuffer);
	for (auto& t : realObj->textures) {
		glDeleteTextures(1, &t);
	}
	for (auto& n : realObj->nodes)
	{
		delete n;
	}
	for (auto& nl : realObj->linearNodes)
	{
		delete nl;
	}
	

	delete realObj;
}






























































const char* pbrVertexShader = "#version 300 es\n\
\n\
layout(location = 0) in vec3 inPos;\n\
layout(location = 1) in vec3 inVNormal;\n\
layout(location = 2) in vec2 inVUV0;\n\
layout(location = 3) in vec2 inVUV1;\n\
layout(location = 4) in vec4 inJoint0;\n\
layout(location = 5) in vec4 inWeight0;\n\
\n\
uniform UBO\n\
{\n\
	mat4 projection;\n\
	mat4 view;\n\
	vec3 camPos;\n\
} ubo;\n\
uniform mat4 model;\n\
uniform vec4 clipPlane;\n\
\n\
#define MAX_NUM_JOINTS 128\n\
\n\
uniform UBONode {\n\
	mat4 matrix;\n\
	mat4 jointMatrix[MAX_NUM_JOINTS];\n\
	float jointCount;\n\
} node;\n\
\n\
out vec3 inWorldPos;\n\
out vec3 inNormal;\n\
out vec2 inUV0;\n\
out vec2 inUV1;\n\
out float clipDist;\n\
\n\
void main()\n\
{\n\
	vec4 locPos;\n\
	if (node.jointCount > 0.0) {\n\
		// Mesh is skinned\n\
		mat4 skinMat =\n\
			inWeight0.x * node.jointMatrix[int(inJoint0.x)] +\n\
			inWeight0.y * node.jointMatrix[int(inJoint0.y)] +\n\
			inWeight0.z * node.jointMatrix[int(inJoint0.z)] +\n\
			inWeight0.w * node.jointMatrix[int(inJoint0.w)];\n\
	\n\
		locPos = model * node.matrix * skinMat * vec4(inPos, 1.0);\n\
		inNormal = normalize(transpose(inverse(mat3(model * node.matrix * skinMat))) * inVNormal);\n\
	}\n\
	else {\n\
		locPos = model * node.matrix * vec4(inPos, 1.0);\n\
		inNormal = normalize(transpose(inverse(mat3(model * node.matrix))) * inVNormal);\n\
	}\n\
	inWorldPos = locPos.xyz / locPos.w;\n\
	inUV0 = inVUV0;\n\
	inUV1 = inVUV1;\n\
	clipDist = dot(vec4(inWorldPos, 1.0f), clipPlane);\n\
	gl_Position = ubo.projection * ubo.view * vec4(inWorldPos, 1.0);\n\
}";


const char* pbrFragmentShaderExtension = "\n\
in vec3 inWorldPos;\n\
in vec3 inNormal;\n\
in vec2 inUV0;\n\
in vec2 inUV1;\n\
in float clipDist;\n\
\n\
\n\
\n\
uniform UBO {\n\
	mat4 projection;\n\
	mat4 view;\n\
	vec3 camPos;\n\
} ubo;\n\
uniform mat4 model;\n\
\n\
uniform UBOParams {\n\
	vec4 lightDir;\n\
	float exposure;\n\
	float gamma;\n\
	float prefilteredCubeMipLevels;\n\
	float scaleIBLAmbient;\n\
	float debugViewInputs;\n\
	float debugViewEquation;\n\
} uboParams;\n\
\n\
uniform samplerCube samplerIrradiance;\n\
uniform samplerCube prefilteredMap;\n\
uniform sampler2D samplerBRDFLUT;\n\
\n\
\n\
\n\
uniform sampler2D colorMap;\n\
uniform sampler2D physicalDescriptorMap;\n\
uniform sampler2D normalMap;\n\
uniform sampler2D aoMap;\n\
uniform sampler2D emissiveMap;\n\
\n\
uniform Material {\n\
	vec4 baseColorFactor;\n\
	vec4 emissiveFactor;\n\
	vec4 diffuseFactor;\n\
	vec4 specularFactor;\n\
	float workflow;\n\
	int baseColorTextureSet;\n\
	int physicalDescriptorTextureSet;\n\
	int normalTextureSet;\n\
	int occlusionTextureSet;\n\
	int emissiveTextureSet;\n\
	float metallicFactor;\n\
	float roughnessFactor;\n\
	float alphaMask;\n\
	float alphaMaskCutoff;\n\
} material;\n\
\n\
out vec4 outColor;\n\
\n\
\n\
\n\
\n\
struct PBRInfo\n\
{\n\
	float NdotL;                  \n\
	float NdotV;                  \n\
	float NdotH;                  \n\
	float LdotH;                  \n\
	float VdotH;                  \n\
	float perceptualRoughness;    \n\
	float metalness;              \n\
	vec3 reflectance0;            \n\
	vec3 reflectance90;           \n\
	float alphaRoughness;         \n\
	vec3 diffuseColor;            \n\
	vec3 specularColor;           \n\
};\n\
\n\
const float M_PI = 3.141592653589793;\n\
const float c_MinRoughness = 0.04;\n\
\n\
const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;\n\
const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;\n\
\n\
#define MANUAL_SRGB 1\n\
\n\
vec3 Uncharted2Tonemap(vec3 color)\n\
{\n\
	float A = 0.15;\n\
	float B = 0.50;\n\
	float C = 0.10;\n\
	float D = 0.20;\n\
	float E = 0.02;\n\
	float F = 0.30;\n\
	float W = 11.2;\n\
	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;\n\
}\n\
\n\
vec4 tonemap(vec4 color)\n\
{\n\
	vec3 outcol = Uncharted2Tonemap(color.rgb * uboParams.exposure);\n\
	outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));\n\
	return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);\n\
}\n\
\n\
vec4 SRGBtoLINEAR(vec4 srgbIn)\n\
{\n\
#ifdef MANUAL_SRGB\n\
#ifdef SRGB_FAST_APPROXIMATION\n\
	vec3 linOut = pow(srgbIn.xyz, vec3(2.2f));\n\
#else //SRGB_FAST_APPROXIMATION\n\
	vec3 bLess = step(vec3(0.04045f), srgbIn.xyz);\n\
	vec3 linOut = mix(srgbIn.xyz / vec3(12.92f), pow((srgbIn.xyz + vec3(0.055f)) / vec3(1.055f), vec3(2.4f)), bLess);\n\
#endif //SRGB_FAST_APPROXIMATION\n\
	return vec4(linOut, srgbIn.w);\n\
#else //MANUAL_SRGB\n\
	return srgbIn;\n\
#endif //MANUAL_SRGB\n\
}\n\
\n\
vec3 getNormal()\n\
{\n\
\n\
	vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0f - 1.0f;\n\
\n\
	vec3 q1 = dFdx(inWorldPos);\n\
	vec3 q2 = dFdy(inWorldPos);\n\
	vec2 st1 = dFdx(inUV0);\n\
	vec2 st2 = dFdy(inUV0);\n\
\n\
	vec3 N = normalize(inNormal);\n\
	vec3 T = normalize(q1 * st2.t - q2 * st1.t);\n\
	vec3 B = -normalize(cross(N, T));\n\
	mat3 TBN = mat3(T, B, N);\n\
\n\
	return normalize(TBN * tangentNormal);\n\
}\n\
\n\
\n\
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)\n\
{\n\
	float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);\n\
	// retrieve a scale and bias to F0. See [1], Figure 3\n\
	vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0f - pbrInputs.perceptualRoughness))).rgb;\n\
	vec3 diffuseLight = SRGBtoLINEAR(tonemap(texture(samplerIrradiance, n))).rgb;\n\
\n\
	vec3 specularLight = SRGBtoLINEAR(tonemap(textureLod(prefilteredMap, reflection, lod))).rgb;\n\
\n\
	vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;\n\
	vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);\n\
\n\
	diffuse *= uboParams.scaleIBLAmbient;\n\
	specular *= uboParams.scaleIBLAmbient;\n\
\n\
	return diffuse + specular;\n\
}\n\
\n\
vec3 diffuse(PBRInfo pbrInputs)\n\
{\n\
	return pbrInputs.diffuseColor / M_PI;\n\
}\n\
\n\
vec3 specularReflection(PBRInfo pbrInputs)\n\
{\n\
	return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0f - pbrInputs.VdotH, 0.0f, 1.0f), 5.0f);\n\
}\n\
\n\
float geometricOcclusion(PBRInfo pbrInputs)\n\
{\n\
	float NdotL = pbrInputs.NdotL;\n\
	float NdotV = pbrInputs.NdotV;\n\
	float r = pbrInputs.alphaRoughness;\n\
\n\
	float attenuationL = 2.0f * NdotL / (NdotL + sqrt(r * r + (1.0f - r * r) * (NdotL * NdotL)));\n\
	float attenuationV = 2.0f * NdotV / (NdotV + sqrt(r * r + (1.0f - r * r) * (NdotV * NdotV)));\n\
	return attenuationL * attenuationV;\n\
}\n\
\n\
float microfacetDistribution(PBRInfo pbrInputs)\n\
{\n\
	float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;\n\
	float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;\n\
	return roughnessSq / (M_PI * f * f);\n\
}\n\
\n\
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {\n\
	float perceivedDiffuse = sqrt(0.299f * diffuse.r * diffuse.r + 0.587f * diffuse.g * diffuse.g + 0.114f * diffuse.b * diffuse.b);\n\
	float perceivedSpecular = sqrt(0.299f * specular.r * specular.r + 0.587f * specular.g * specular.g + 0.114f * specular.b * specular.b);\n\
	if (perceivedSpecular < c_MinRoughness) {\n\
		return 0.0;\n\
	}\n\
	float a = c_MinRoughness;\n\
	float b = perceivedDiffuse * (1.0f - maxSpecular) / (1.0f - c_MinRoughness) + perceivedSpecular - 2.0f * c_MinRoughness;\n\
	float c = c_MinRoughness - perceivedSpecular;\n\
	float D = max(b * b - 4.0 * a * c, 0.0f);\n\
	return clamp((-b + sqrt(D)) / (2.0f * a), 0.0f, 1.0f);\n\
}\n\
\n\
void main()\n\
{\n\
	if(clipDist < 0.0f) discard;\n\
	float perceptualRoughness;\n\
	float metallic;\n\
	vec3 diffuseColor;\n\
	vec4 baseColor;\n\
\n\
	vec3 f0 = vec3(0.04f);\n\
\n\
	if (material.alphaMask == 1.0f) {\n\
		if (material.baseColorTextureSet > -1) {\n\
			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;\n\
		}\n\
		else {\n\
			baseColor = material.baseColorFactor;\n\
		}\n\
		if (baseColor.a < material.alphaMaskCutoff) {\n\
			discard;\n\
		}\n\
	}\n\
\n\
	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {\n\
		perceptualRoughness = material.roughnessFactor;\n\
		metallic = material.metallicFactor;\n\
		if (material.physicalDescriptorTextureSet > -1) {\n\
			vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);\n\
			perceptualRoughness = mrSample.g * perceptualRoughness;\n\
			metallic = mrSample.b * metallic;\n\
		}\n\
		else {\n\
			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);\n\
			metallic = clamp(metallic, 0.0, 1.0);\n\
		}\n\
		if (material.baseColorTextureSet > -1) {\n\
			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;\n\
		}\n\
		else {\n\
			baseColor = material.baseColorFactor;\n\
		}\n\
	}\n\
\n\
	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {\n\
		// Values from specular glossiness workflow are converted to metallic roughness\n\
		if (material.physicalDescriptorTextureSet > -1) {\n\
			perceptualRoughness = 1.0 - texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;\n\
		}\n\
		else {\n\
			perceptualRoughness = 0.0;\n\
		}\n\
\n\
		const float epsilon = 1e-6;\n\
\n\
		vec4 diffuse = SRGBtoLINEAR(texture(colorMap, inUV0));\n\
		vec3 specular = SRGBtoLINEAR(texture(physicalDescriptorMap, inUV0)).rgb;\n\
\n\
		float maxSpecular = max(max(specular.r, specular.g), specular.b);\n\
\n\
		// Convert metallic value from specular glossiness inputs\n\
		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);\n\
\n\
		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0f - maxSpecular) / (1.0f - c_MinRoughness) / max(1.0f - metallic, epsilon)) * material.diffuseFactor.rgb;\n\
		vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1.0f - metallic) * (1.0f / max(metallic, epsilon))) * material.specularFactor.rgb;\n\
		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);\n\
	}\n\
\n\
	diffuseColor = baseColor.rgb * (vec3(1.0f) - f0);\n\
	diffuseColor *= 1.0f - metallic;\n\
\n\
	float alphaRoughness = perceptualRoughness * perceptualRoughness;\n\
\n\
	vec3 specularColor = mix(f0, baseColor.rgb, metallic);\n\
\n\
	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);\n\
\n\
	float reflectance90 = clamp(reflectance * 25.0f, 0.0f, 1.0f);\n\
	vec3 specularEnvironmentR0 = specularColor.rgb;\n\
	vec3 specularEnvironmentR90 = vec3(1.0f, 1.0f, 1.0f) * reflectance90;\n\
\n\
	vec3 n = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);\n\
	vec3 v = normalize(ubo.camPos - inWorldPos);    // Vector from surface point to camera\n\
	vec3 l = normalize(uboParams.lightDir.xyz);     // Vector from surface point to light\n\
	vec3 h = normalize(l + v);                        // Half vector between both l and v\n\
	vec3 reflection = -normalize(reflect(v, n));\n\
	reflection.y *= -1.0f;\n\
\n\
	float NdotL = clamp(dot(n, l), 0.001, 1.0);\n\
	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);\n\
	float NdotH = clamp(dot(n, h), 0.0, 1.0);\n\
	float LdotH = clamp(dot(l, h), 0.0, 1.0);\n\
	float VdotH = clamp(dot(v, h), 0.0, 1.0);\n\
\n\
	PBRInfo pbrInputs = PBRInfo(\n\
		NdotL,\n\
		NdotV,\n\
		NdotH,\n\
		LdotH,\n\
		VdotH,\n\
		perceptualRoughness,\n\
		metallic,\n\
		specularEnvironmentR0,\n\
		specularEnvironmentR90,\n\
		alphaRoughness,\n\
		diffuseColor,\n\
		specularColor\n\
	);\n\
\n\
	// Calculate the shading terms for the microfacet specular shading model\n\
	vec3 F = specularReflection(pbrInputs);\n\
	float G = geometricOcclusion(pbrInputs);\n\
	float D = microfacetDistribution(pbrInputs);\n\
	vec3 color = CalculateLightsColor(vec4(inWorldPos, 1.0f), n, v, diffuseColor, specularColor, reflectance90);\n\
\n\
	// Calculate lighting contribution from image based lighting source (IBL)\n\
	color += getIBLContribution(pbrInputs, n, reflection);\n\
\n\
	const float u_OcclusionStrength = 1.0f;\n\
	if (material.occlusionTextureSet > -1) {\n\
		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;\n\
		color = mix(color, color * ao, u_OcclusionStrength);\n\
	}\n\
\n\
	const float u_EmissiveFactor = 1.0f;\n\
	if (material.emissiveTextureSet > -1) {\n\
		vec3 emissive = SRGBtoLINEAR(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb * u_EmissiveFactor;\n\
		color += emissive;\n\
	}\n\
\n\
	outColor = vec4(color, baseColor.a);\n\
\n\
	// Shader inputs debug visualization\n\
	if (uboParams.debugViewInputs > 0.0) {\n\
		int index = int(uboParams.debugViewInputs);\n\
		switch (index) {\n\
		case 1:\n\
			outColor.rgba = material.baseColorTextureSet > -1 ? texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) : vec4(1.0f);\n\
			break;\n\
		case 2:\n\
			outColor.rgb = (material.normalTextureSet > -1) ? texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rgb : normalize(inNormal);\n\
			break;\n\
		case 3:\n\
			outColor.rgb = (material.occlusionTextureSet > -1) ? texture(aoMap, material.occlusionTextureSet == 0 ? inUV0 : inUV1).rrr : vec3(0.0f);\n\
			break;\n\
		case 4:\n\
			outColor.rgb = (material.emissiveTextureSet > -1) ? texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb : vec3(0.0f);\n\
			break;\n\
		case 5:\n\
			outColor.rgb = texture(physicalDescriptorMap, inUV0).bbb;\n\
			break;\n\
		case 6:\n\
			outColor.rgb = texture(physicalDescriptorMap, inUV0).ggg;\n\
			break;\n\
		}\n\
		outColor = SRGBtoLINEAR(outColor);\n\
	}\n\
\n\
	// PBR equation debug visualization\n\
	if (uboParams.debugViewEquation > 0.0) {\n\
		int index = int(uboParams.debugViewEquation);\n\
		switch (index) {\n\
		case 1:\n\
			outColor.rgb = F;\n\
			break;\n\
		case 2:\n\
			outColor.rgb = vec3(G);\n\
			break;\n\
		case 3:\n\
			outColor.rgb = vec3(D);\n\
			break;\n\
		}\n\
	}\n\
\n\
}";


// uniform samplerCube samplerIrradiance;
// uniform samplerCube prefilteredMap; 
// uniform sampler2D samplerBRDFLUT; 
// uniform sampler2D colorMap;
// uniform sampler2D physicalDescriptorMap; 
// uniform sampler2D normalMap; 
// uniform sampler2D aoMap; 
// uniform sampler2D emissiveMap;
// uniform sampler2DShadow shadowMap;
enum GLTF_Textures
{
	IRRADIANCE,
	PREFILTERED_MAP,
	BRDF_LUT,
	COLOR_MAP,
	PHYSICAL_DESCRIPTOR_MAP,
	NORMAL_MAP,
	AO_MAP,
	EMISSIVE_MAP,
	SHADOW_MAP,
};

struct GLTFUniforms
{
	GLuint UBOLoc = -1;
	GLuint ModelMatrixLoc = -1;
	GLuint UBONodeLoc = -1;
	GLuint UBOParamsLoc = -1;
	GLuint MaterialLoc = -1;
	GLuint clipPlaneLoc = -1;
	GLuint lightDataLoc = -1;
};

struct OpenGlPipelineObjects
{
	GLuint geomProgram;
	GLuint shaderProgram;
	GLuint brdfLutMap;
	GLuint defTex;
	GLuint SharedUBOUniform;
	GLTFUniforms unis;
	GLTFUniforms geomUnis;
	MaterialPbr* currentBoundMaterial = nullptr;
}g_pipeline;

void InitializePbrPipeline(void* assetManager)
{
#ifdef ANDROID
	tinygltf::asset_manager = (AAssetManager*)assetManager;
#endif
	g_pipeline.geomProgram = CreateProgram(pbrVertexShader);
	g_pipeline.shaderProgram = CreateProgramExtended(pbrVertexShader, pbrFragmentShaderExtension, &g_pipeline.unis.lightDataLoc, SHADOW_MAP);
	glGenTextures(1, &g_pipeline.defTex);
	glBindTexture(GL_TEXTURE_2D, g_pipeline.defTex);
	uint32_t defTexColorData[2];
	memset(defTexColorData, 0xFF, 2 * sizeof(uint32_t));
	glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, 1, 1, 0, GL_RGBA, GL_UNSIGNED_BYTE, defTexColorData);
	
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
	glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
	glBindTexture(GL_TEXTURE_2D, 0);


	g_pipeline.unis.UBOLoc = glGetUniformBlockIndex(g_pipeline.shaderProgram, "UBO");
	glUniformBlockBinding(g_pipeline.shaderProgram, g_pipeline.unis.UBOLoc, g_pipeline.unis.UBOLoc);
	g_pipeline.unis.UBONodeLoc = glGetUniformBlockIndex(g_pipeline.shaderProgram, "UBONode");
	glUniformBlockBinding(g_pipeline.shaderProgram, g_pipeline.unis.UBONodeLoc, g_pipeline.unis.UBONodeLoc);
	g_pipeline.unis.UBOParamsLoc = glGetUniformBlockIndex(g_pipeline.shaderProgram, "UBOParams");
	glUniformBlockBinding(g_pipeline.shaderProgram, g_pipeline.unis.UBOParamsLoc, g_pipeline.unis.UBOParamsLoc);
	g_pipeline.unis.MaterialLoc = glGetUniformBlockIndex(g_pipeline.shaderProgram, "Material");
	glUniformBlockBinding(g_pipeline.shaderProgram, g_pipeline.unis.MaterialLoc, g_pipeline.unis.MaterialLoc);
	g_pipeline.unis.ModelMatrixLoc = glGetUniformLocation(g_pipeline.shaderProgram, "model");
	g_pipeline.unis.clipPlaneLoc = glGetUniformLocation(g_pipeline.shaderProgram, "clipPlane");

	g_pipeline.geomUnis.UBOLoc = glGetUniformBlockIndex(g_pipeline.geomProgram, "UBO");
	glUniformBlockBinding(g_pipeline.geomProgram, g_pipeline.geomUnis.UBOLoc, g_pipeline.geomUnis.UBOLoc);
	g_pipeline.geomUnis.UBONodeLoc = glGetUniformBlockIndex(g_pipeline.geomProgram, "UBONode");
	glUniformBlockBinding(g_pipeline.geomProgram, g_pipeline.geomUnis.UBONodeLoc, g_pipeline.geomUnis.UBONodeLoc);
	g_pipeline.geomUnis.UBOParamsLoc = glGetUniformBlockIndex(g_pipeline.geomProgram, "UBOParams");
	glUniformBlockBinding(g_pipeline.geomProgram, g_pipeline.geomUnis.UBOParamsLoc, g_pipeline.geomUnis.UBOParamsLoc);
	g_pipeline.geomUnis.MaterialLoc = glGetUniformBlockIndex(g_pipeline.geomProgram, "Material");
	glUniformBlockBinding(g_pipeline.geomProgram, g_pipeline.geomUnis.MaterialLoc, g_pipeline.geomUnis.MaterialLoc);
	g_pipeline.geomUnis.ModelMatrixLoc = glGetUniformLocation(g_pipeline.geomProgram, "model");
	g_pipeline.geomUnis.clipPlaneLoc = glGetUniformLocation(g_pipeline.geomProgram, "clipPlane");
	g_pipeline.geomUnis.lightDataLoc = -1;

	glGenBuffers(1, &g_pipeline.SharedUBOUniform);

	//LOG("UBO/UBONode/UBOParams/Material Locs: %d, %d, %d, %d\n", g_pipeline.UBOLoc, g_pipeline.UBONodeLoc, g_pipeline.UBOParamsLoc, g_pipeline.MaterialLoc);

	glUseProgram(g_pipeline.shaderProgram);
	GLint curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "samplerIrradiance");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, IRRADIANCE);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "prefilteredMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, PREFILTERED_MAP);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "samplerBRDFLUT");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, BRDF_LUT);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "colorMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, COLOR_MAP);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "physicalDescriptorMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, PHYSICAL_DESCRIPTOR_MAP);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "normalMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, NORMAL_MAP);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "aoMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, AO_MAP);
	curTexture = glGetUniformLocation(g_pipeline.shaderProgram, "emissiveMap");
	if (curTexture == -1) { LOG("failed to Get Texture Loaction of GLTF shader program\n"); }
	glUniform1i(curTexture, EMISSIVE_MAP);
	glUseProgram(0);

	g_pipeline.brdfLutMap = GenerateBRDF_LUT(512);
}
void CleanUpPbrPipeline()
{
	glDeleteTextures(1, &g_pipeline.brdfLutMap);
	glDeleteTextures(1, &g_pipeline.defTex);
	glDeleteProgram(g_pipeline.shaderProgram);
	g_pipeline.brdfLutMap = 0; g_pipeline.defTex = 0; g_pipeline.shaderProgram = 0;
	g_pipeline.currentBoundMaterial = nullptr;
}


void BindMaterial(MaterialPbr* mat, GLTFUniforms* unis)
{
	glBindBuffer(GL_UNIFORM_BUFFER, mat->uniformBuffer);
	glBindBufferBase(GL_UNIFORM_BUFFER, unis->MaterialLoc, mat->uniformBuffer);

	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::AO_MAP);
	glBindTexture(GL_TEXTURE_2D, mat->occlusionTexture ? mat->occlusionTexture : g_pipeline.defTex);
	
	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::EMISSIVE_MAP);
	glBindTexture(GL_TEXTURE_2D, mat->emissiveTexture ? mat->emissiveTexture : g_pipeline.defTex);

	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::NORMAL_MAP);
	glBindTexture(GL_TEXTURE_2D, mat->normalTexture ? mat->normalTexture : g_pipeline.defTex);

	if (mat->pbrWorkflows.metallicRoughness)
	{
		glActiveTexture(GL_TEXTURE0 + GLTF_Textures::COLOR_MAP);
		glBindTexture(GL_TEXTURE_2D, mat->baseColorTexture ? mat->baseColorTexture : g_pipeline.defTex);

		glActiveTexture(GL_TEXTURE0 + GLTF_Textures::PHYSICAL_DESCRIPTOR_MAP);
		glBindTexture(GL_TEXTURE_2D, mat->metallicRoughnessTexture ? mat->metallicRoughnessTexture : g_pipeline.defTex);
	}
	else if (mat->pbrWorkflows.specularGlossiness)
	{
		glActiveTexture(GL_TEXTURE0 + GLTF_Textures::COLOR_MAP);
		glBindTexture(GL_TEXTURE_2D, mat->extension.diffuseTexture ? mat->extension.diffuseTexture : g_pipeline.defTex);

		glActiveTexture(GL_TEXTURE0 + GLTF_Textures::PHYSICAL_DESCRIPTOR_MAP);
		glBindTexture(GL_TEXTURE_2D, mat->extension.specularGlossinessTexture ? mat->extension.specularGlossinessTexture : g_pipeline.defTex);
	}
	else
	{
		glActiveTexture(GL_TEXTURE0 + GLTF_Textures::COLOR_MAP);
		glBindTexture(GL_TEXTURE_2D, g_pipeline.defTex);
	}
	
}
void DrawNode(InternalPBR* realObj, Node* node, bool drawOpaque, GLTFUniforms* unis)
{
	if (node->mesh) {
		glBindBufferRange(GL_UNIFORM_BUFFER, unis->UBONodeLoc, node->mesh->uniformBuffer.handle, 0, sizeof(MeshPbr::UniformBlock));
		for (Primitive* primitive : node->mesh->primitives)
		{
			if (drawOpaque && primitive->material.alphaMode == MaterialPbr::ALPHAMODE_BLEND) continue;
			else if (!drawOpaque && primitive->material.alphaMode != MaterialPbr::ALPHAMODE_BLEND) continue;

			if (g_pipeline.currentBoundMaterial != &primitive->material)
			{
				BindMaterial(&primitive->material, unis);
				g_pipeline.currentBoundMaterial = &primitive->material;
			}
			glDrawElementsWrapper(GL_TRIANGLES, primitive->indexCount, GL_UNSIGNED_INT, (const void*)(primitive->firstIndex * sizeof(uint32_t)));
		}
	}
	for (auto& child : node->children) {
		DrawNode(realObj, child, drawOpaque, unis);
	}
}
void DrawPBRModel(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint environmentMap, GLuint lightDataUniform, const glm::mat4* model, const glm::vec4* clipPlane, bool drawOpaque, bool geomOnly)
{
	GLTFUniforms* unis = nullptr;
	if (geomOnly) unis = &g_pipeline.geomUnis;
	else unis = &g_pipeline.unis;
	g_pipeline.currentBoundMaterial = nullptr;

	InternalPBR* realObj = (InternalPBR*)internalObj;

	glUseProgramWrapper(geomOnly ? g_pipeline.geomProgram : g_pipeline.shaderProgram);

	glBindVertexArray(realObj->vao);

	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, realObj->indices.indexBuffer);

	
	glBindBufferBase(GL_UNIFORM_BUFFER, unis->UBOLoc, UboUniform);
	
	glUniformMatrix4fv(unis->ModelMatrixLoc, 1, GL_FALSE, (const GLfloat*)model);
	glBindBufferRange(GL_UNIFORM_BUFFER, unis->UBOParamsLoc, UBOParamsUniform, 0, sizeof(UBOParams));

	if (!geomOnly)
		glBindBufferBase(GL_UNIFORM_BUFFER, unis->lightDataLoc, lightDataUniform);

	if (clipPlane) glUniform4fv(unis->clipPlaneLoc, 1, (const GLfloat*)clipPlane);
	else glUniform4f(unis->clipPlaneLoc, 0.0f, 1.0f, 0.0f, 100000000.0f);


	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::PREFILTERED_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);

	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::IRRADIANCE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);


	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::BRDF_LUT);
	glBindTexture(GL_TEXTURE_2D, g_pipeline.brdfLutMap);

	for (auto& node : realObj->nodes) {
		DrawNode(realObj, node, drawOpaque, unis);
	}
}
void DrawPBRModelNode(void* internalObj, GLuint UboUniform, GLuint UBOParamsUniform, GLuint environmentMap, GLuint lightDataUniform, const glm::mat4* model, const glm::vec4* clipPlane, int nodeIdx, bool drawOpaque, bool geomOnly)
{
	GLTFUniforms* unis = nullptr;
	if (geomOnly) unis = &g_pipeline.geomUnis;
	else unis = &g_pipeline.unis;

	g_pipeline.currentBoundMaterial = nullptr;
	InternalPBR* realObj = (InternalPBR*)internalObj;
	if (realObj->nodes.size() <= nodeIdx) return;

	glUseProgramWrapper(g_pipeline.shaderProgram);
	glBindVertexArray(realObj->vao);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, realObj->indices.indexBuffer);
	glBindBufferRange(GL_UNIFORM_BUFFER, unis->UBOLoc, UboUniform, 0, sizeof(UBO));
	glUniformMatrix4fv(unis->ModelMatrixLoc, 1, GL_FALSE, (const GLfloat*)model);
	glBindBufferRange(GL_UNIFORM_BUFFER, unis->UBOParamsLoc, UBOParamsUniform, 0, sizeof(UBOParams));

	if (!geomOnly)
		glBindBufferBase(GL_UNIFORM_BUFFER, unis->lightDataLoc, lightDataUniform);

	if (clipPlane) glUniform4fv(unis->clipPlaneLoc, 1, (const GLfloat*)clipPlane);
	else glUniform4f(unis->clipPlaneLoc, 0.0f, 1.0f, 0.0f, 100000000.0f);

	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::PREFILTERED_MAP);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);

	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::IRRADIANCE);
	glBindTexture(GL_TEXTURE_CUBE_MAP, environmentMap);


	glActiveTexture(GL_TEXTURE0 + GLTF_Textures::BRDF_LUT);
	glBindTexture(GL_TEXTURE_2D, g_pipeline.brdfLutMap);

	DrawNode(realObj, realObj->nodes.at(nodeIdx), drawOpaque, unis);
}

void UpdateAnimation(void* internalObj, uint32_t index, float time)
{
	InternalPBR* realObj = (InternalPBR*)internalObj;
	if (realObj->animations.empty()) {
		LOG("NO ANIMATION AVAILABLE FOR GLTF MODEL\n");
		return;
	}
	if (index > (realObj->animations.size() - 1)) {
		LOG("ANIMATION INDEX OUT OF BOUND %d\n", index);
		return;
	}

	Animation& animation = realObj->animations[index];
	bool updated = false;
	
	for (auto& channel : animation.channels) {
		AnimationSampler& sampler = animation.samplers[channel.samplerIndex];
		if (sampler.inputs.size() > sampler.outputsVec4.size()) {
			continue;
		}

		for (size_t i = 0; i < sampler.inputs.size() - 1; i++) {
			if ((realObj->animationTime >= sampler.inputs[i]) && (realObj->animationTime <= sampler.inputs[i + 1])) {
				float u = std::max(0.0f, realObj->animationTime - sampler.inputs[i]) / (sampler.inputs[i + 1] - sampler.inputs[i]);
				if (u <= 1.0f) {
					switch (channel.path) {
					case AnimationChannel::PathType::TRANSLATION: {
						glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
						channel.node->translation = glm::vec3(trans);
						break;
					}
					case AnimationChannel::PathType::SCALE: {
						glm::vec4 trans = glm::mix(sampler.outputsVec4[i], sampler.outputsVec4[i + 1], u);
						channel.node->scale = glm::vec3(trans);
						break;
					}
					case AnimationChannel::PathType::ROTATION: {
						glm::quat q1;
						q1.x = sampler.outputsVec4[i].x;
						q1.y = sampler.outputsVec4[i].y;
						q1.z = sampler.outputsVec4[i].z;
						q1.w = sampler.outputsVec4[i].w;
						glm::quat q2;
						q2.x = sampler.outputsVec4[i + 1].x;
						q2.y = sampler.outputsVec4[i + 1].y;
						q2.z = sampler.outputsVec4[i + 1].z;
						q2.w = sampler.outputsVec4[i + 1].w;
						channel.node->rotation = glm::normalize(glm::slerp(q1, q2, u));
						break;
					}
					}
					updated = true;
				}
			}
		}
	}
	if (updated) 
	{
		realObj->animationTime += time;
		for (auto& node : realObj->nodes) {
			node->update();
		}
	}
	else 
	{
		realObj->animationTime = 0.0f;
	}
}

void DrawScenePBRGeometry(SceneObject* obj, void* renderPassData)
{
	PBRSceneObject* o = (PBRSceneObject*)obj;
	StandardRenderPassData* passData = (StandardRenderPassData*)renderPassData;
	DrawPBRModel(o->model, passData->cameraUniform, o->uboParamsUniform, passData->skyBox, passData->lightData, o->transform, nullptr, true, true);
}
void DrawScenePBROpaque(SceneObject* obj, void* renderPassData)
{
	PBRSceneObject* o = (PBRSceneObject*)obj;
	StandardRenderPassData* passData = (StandardRenderPassData*)renderPassData;
	SetDefaultOpaqueState();
	DrawPBRModel(o->model, passData->cameraUniform, o->uboParamsUniform, passData->skyBox, passData->lightData, o->transform, nullptr, true, false);

}
void DrawScenePBRBlend(SceneObject* obj, void* renderPassData)
{
	PBRSceneObject* o = (PBRSceneObject*)obj;
	StandardRenderPassData* passData = (StandardRenderPassData*)renderPassData;
	SetDefaultBlendState();
	DrawPBRModel(o->model, passData->cameraUniform, o->uboParamsUniform, passData->skyBox, passData->lightData, o->transform, nullptr, false, false);
}
void DrawScenePBROpaqueClip(SceneObject* obj, void* renderPassData)
{
	PBRSceneObject* o = (PBRSceneObject*)obj;
	ReflectPlanePassData* passData = (ReflectPlanePassData*)renderPassData;
	SetDefaultOpaqueState();
	DrawPBRModel(o->model, passData->base->cameraUniform, o->uboParamsUniform, passData->base->skyBox, passData->base->lightData, o->transform, passData->planeEquation, true, false);
}
void DrawScenePBRBlendClip(SceneObject* obj, void* renderPassData)
{
	PBRSceneObject* o = (PBRSceneObject*)obj; 
	ReflectPlanePassData* passData = (ReflectPlanePassData*)renderPassData;
	SetDefaultBlendState();
	DrawPBRModel(o->model, passData->base->cameraUniform, o->uboParamsUniform, passData->base->skyBox, passData->base->lightData, o->transform, passData->planeEquation, false, false);
}

PFUNCDRAWSCENEOBJECT PBRModelGetDrawFunction(TYPE_FUNCTION f)
{
	if (f == TYPE_FUNCTION::TYPE_FUNCTION_GEOMETRY) return DrawScenePBRGeometry;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_OPAQUE) return DrawScenePBROpaque;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_BLEND) return DrawScenePBRBlend;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_OPAQUE) return DrawScenePBROpaqueClip;
	else if (f == TYPE_FUNCTION::TYPE_FUNCTION_CLIP_PLANE_BLEND) return DrawScenePBRBlendClip;
	return nullptr;
}


PBRSceneObject* AddPbrModelToScene(PScene scene, void* internalObj, UBOParams params, const glm::mat4& transform)
{
	InternalPBR* realObj = (InternalPBR*)internalObj;
	PBRSceneObject* pbr = (PBRSceneObject*)SC_AddSceneObject(scene, PBR_RENDERABLE);
	pbr->transform = (glm::mat4*)SC_AddTransform(scene, PBR_RENDERABLE, sizeof(glm::mat4));
	*pbr->transform = transform;

	pbr->model = internalObj;

	glm::vec3 min(FLT_MAX);
	glm::vec3 max(-FLT_MAX);

	for (int i = 0; i < realObj->nodes.size(); i++)
	{
		BoundingBoxPbr pbrBB = realObj->nodes.at(i)->mesh->bb.getAABB(transform);
		
		glm::vec3& minRes = pbrBB.min;
		glm::vec3& maxRes = pbrBB.max;

		if (minRes.x < min.x) min.x = minRes.x;
		if (maxRes.x < min.x) min.x = maxRes.x;
		if (minRes.x > max.x) max.x = minRes.x;
		if (maxRes.x > max.x) max.x = maxRes.x;
		
		if (minRes.y < min.y) min.y = minRes.y;
		if (maxRes.y < min.y) min.y = maxRes.y;
		if (minRes.y > max.y) max.y = minRes.y;
		if (maxRes.y > max.y) max.y = maxRes.y;

		if (minRes.z < min.z) min.z = minRes.z;
		if (maxRes.z < min.z) min.z = maxRes.z;
		if (minRes.z > max.z) max.z = minRes.z;
		if (maxRes.z > max.z) max.z = maxRes.z;
	}

	pbr->base.bbox.leftTopFront = min;
	pbr->base.bbox.rightBottomBack = max;

	glGenBuffers(1, &pbr->uboParamsUniform);
	glBindBuffer(GL_UNIFORM_BUFFER, pbr->uboParamsUniform);
	glBufferData(GL_UNIFORM_BUFFER, sizeof(UBOParams), &params, GL_STATIC_DRAW);
	
	pbr->base.flags = SCENE_OBJECT_OPAQUE | SCENE_OBJECT_BLEND | SCENE_OBJECT_CAST_SHADOW | SCENE_OBJECT_REFLECTED | SCENE_OBJECT_SURFACE_REFLECTED;
	return pbr;
}