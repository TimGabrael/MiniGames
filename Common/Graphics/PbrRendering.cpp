#include "PbrRendering.h"
#include "Helper.h"
#include "../logging.h"
#include <vector>

#define GLM_FORCE_RADIANS
#define GLM_FORCE_DEPTH_ZERO_TO_ONE
#include <glm/glm.hpp>
#include <glm/gtc/matrix_transform.hpp>
#include <glm/gtc/type_ptr.hpp>
#include <glm/gtx/string_cast.hpp>


#define MAX_NUM_JOINTS 128u
struct Node;
struct BoundingBox {
	glm::vec3 min;
	glm::vec3 max;
	bool valid = false;
	BoundingBox() { };
	BoundingBox(const glm::vec3& min, const glm::vec3& max) :min(min), max(max) { };
	BoundingBox getAABB(const glm::mat4& m)
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

		return BoundingBox(min, max);
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
struct Material
{
	enum AlphaMode{ ALPHAMODE_OPAQUE, ALPHAMODE_MASK, ALPHAMODE_BLEND };
	AlphaMode alphaMode = ALPHAMODE_OPAQUE;
	float alphaCutoff = 1.0f;
	float metallicFactor = 1.0f;
	float roughnessFactor = 1.0f;
	glm::vec4 baseColorFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	glm::vec4 emissiveFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
	GLuint baseColorTexture;
	GLuint metallicRoughnessTexture;
	GLuint normalTexture;
	GLuint occlusionTexture;
	GLuint emissiveTexture;
	struct TexCoorSets {
		uint8_t baseColor = 0; 
		uint8_t metallicRoughness = 0;
		uint8_t specularGlossiness = 0;
		uint8_t normal = 0;
		uint8_t occlusion = 0;
		uint8_t emissive = 0;
	}texCoordSets;
	struct Extension {
		GLuint specularGlossinessTexture;
		GLuint diffuseTexture;
		glm::vec4 diffuseFactor = { 1.0f, 1.0f, 1.0f, 1.0f };
		glm::vec3 specularFactor = { 0.0f, 0.0f, 0.0f };
	}extension;
	struct PbrWorkflows {
		bool metallicRoughness = true;
		bool specularGlossiness = false;
	}pbrWorkflows;

};
struct Primitive {
	Primitive::Primitive(uint32_t firstIndex, uint32_t indexCount, uint32_t vertexCount, Material& material) : firstIndex(firstIndex), indexCount(indexCount), vertexCount(vertexCount), material(material) {
		hasIndices = indexCount > 0;
	}
	void SetBoundingBox(glm::vec3 min, glm::vec3 max)
	{
		bb.min = min; bb.max = max; bb.valid = true;
	}
	uint32_t firstIndex; uint32_t indexCount; uint32_t vertexCount;
	Material& material;
	bool hasIndices;
	BoundingBox bb;
};

struct Mesh {
	Mesh(const glm::mat4& matrix) {
		uniformBlock.matrix = matrix;
		glGenBuffers(1, &uniformBuffer.handle);
		glBindBuffer(GL_UNIFORM_BUFFER, uniformBuffer.handle);
		glBufferData(GL_UNIFORM_BUFFER, sizeof(uniformBlock), &uniformBlock, GL_DYNAMIC_DRAW);

		uniformBuffer.mapped = glMapBuffer(GL_UNIFORM_BUFFER, GL_READ_WRITE);
		glBindBuffer(GL_UNIFORM_BUFFER, 0);
	}
	void SetBoundingBox(const glm::vec3& min, const glm::vec3& max) {
		bb.min = min; bb.max = max; bb.valid = true;
	}
	Mesh::~Mesh() {
		glDeleteBuffers(1, &uniformBuffer.handle);
		for (Primitive* p : primitives)
			delete p;
	}
	std::vector<Primitive*> primitives;
	BoundingBox bb;
	BoundingBox aabb;
	struct UniformBuffer {
		GLuint handle;
		void* mapped;
	}uniformBuffer;
	struct UniformBlock {
		glm::mat4 matrix;
		glm::mat4 jointMatrix[MAX_NUM_JOINTS]{};
		float jointcount = 0;
	}uniformBlock;

};

struct Node {
	Node* parent;
	uint32_t index;
	std::vector<Node*> children;
	glm::mat4 matrix;
	std::string name;
	Mesh* mesh;
	Skin* skin;
	int32_t skinIndex = -1;
	glm::vec3 translation{};
	glm::vec3 scale{ 1.0f };
	glm::quat rotation{};
	BoundingBox bvh;
	BoundingBox aabb;
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
			glm::mat4 m = getMatrix();
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
				memcpy(mesh->uniformBuffer.mapped, &mesh->uniformBlock, sizeof(mesh->uniformBlock));
			}
			else {
				memcpy(mesh->uniformBuffer.mapped, &m, sizeof(glm::mat4));
			}
		}

		for (auto& child : children) {
			child->update();
		}
	}
	~Node()
	{

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
	GLuint vertexBuffer;
	struct Indices {
		GLuint indexBuffer;
		int count;
	}indices;
	glm::mat4 aabb;
	std::vector<GLuint> textures;
	std::vector<Material> materials;
	std::vector<Node*> nodes;
	std::vector<Node*> linearNodes;
	std::vector<Animation> animations;
	std::vector<Skin*> skins;

	struct Dimensions {
		glm::vec3 min = glm::vec3(FLT_MAX);
		glm::vec3 max = glm::vec3(-FLT_MAX);
	} dimensions;


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
		glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAX_ANISOTROPY, 16);

		if (tex.sampler != -1)
		{
			const tinygltf::Sampler& smpl = m.samplers.at(tex.sampler);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, smpl.wrapS);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, smpl.wrapT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, smpl.minFilter);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, smpl.magFilter);
		}
		else
		{
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_S, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_WRAP_T, GL_REPEAT);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MIN_FILTER, GL_LINEAR);
			glTexParameteri(GL_TEXTURE_2D, GL_TEXTURE_MAG_FILTER, GL_LINEAR);
		}

		glTexImage2D(GL_TEXTURE_2D, 0, GL_RGBA8, img.width, img.height, 0, GL_RGBA, GL_UNSIGNED_BYTE, img.image.data());
		glGenerateMipmap(GL_TEXTURE_2D);
		
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
		Material material{};
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
			double* data = mat.values["baseColorFactor"].ColorFactor().data();
			material.baseColorFactor = { data[0], data[1], data[2], data[3] };
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
				material.alphaMode = Material::ALPHAMODE_BLEND;
			}
			if (param.string_value == "MASK") {
				material.alphaCutoff = 0.5f;
				material.alphaMode = Material::ALPHAMODE_MASK;
			}
		}
		if (mat.additionalValues.find("alphaCutoff") != mat.additionalValues.end()) {
			material.alphaCutoff = static_cast<float>(mat.additionalValues["alphaCutoff"].Factor());
		}
		if (mat.additionalValues.find("emissiveFactor") != mat.additionalValues.end()) {
			double* data = mat.additionalValues["emissiveFactor"].ColorFactor().data();
			material.emissiveFactor = { data[0], data[1], data[2], 1.0 };
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

		pbr.materials.push_back(material);
	}
	pbr.materials.push_back(Material());
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
		newNode->translation = translation;
	}
	glm::mat4 rotation = glm::mat4(1.0f);
	if (node.rotation.size() == 4) {
		glm::quat q = glm::make_quat(node.rotation.data());
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
		Mesh* newMesh = new Mesh(newNode->matrix);
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
					vert.pos = glm::vec4(glm::make_vec3(&bufferPos[v * posByteStride]), 1.0f);
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
							std::cerr << "Joint component type " << jointComponentType << " not supported!" << std::endl;
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
					std::cerr << "Index component type " << accessor.componentType << " not supported!" << std::endl;
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
					std::cout << "unknown type" << std::endl;
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
				std::cout << "weights not yet supported, skipping channel" << std::endl;
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
	BoundingBox parentBvh = parent ? parent->bvh : BoundingBox(pbr.dimensions.min, pbr.dimensions.max);

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

	glGenBuffers(1, &pbr.vertexBuffer);
	glGenBuffers(1, &pbr.indices.indexBuffer);
	
	glBufferData(GL_ARRAY_BUFFER, vertexBufferSize, loaderInfo.vertexBuffer, GL_STATIC_DRAW);
	glBufferData(GL_ELEMENT_ARRAY_BUFFER, indexBufferSize, loaderInfo.indexBuffer, GL_STATIC_DRAW);

	delete[] loaderInfo.vertexBuffer;
	delete[] loaderInfo.indexBuffer;

	return resultPbr;
}
void CleanUpInternal(void* internalObj)
{
	InternalPBR* realObj = (InternalPBR*)internalObj;
	delete realObj;
}
































































const char* pbrVertexShader = "#version 450\n\
\n\
layout(location = 0) in vec3 inPos;\n\
layout(location = 1) in vec3 inNormal;\n\
layout(location = 2) in vec2 inUV0;\n\
layout(location = 3) in vec2 inUV1;\n\
layout(location = 4) in vec4 inJoint0;\n\
layout(location = 5) in vec4 inWeight0;\n\
\n\
layout(set = 0, binding = 0) uniform UBO\n\
{\n\
	mat4 projection;\n\
	mat4 model;\n\
	mat4 view;\n\
	vec3 camPos;\n\
} ubo;\n\
\n\
#define MAX_NUM_JOINTS 128\n\
\n\
layout(set = 2, binding = 0) uniform UBONode {\n\
	mat4 matrix;\n\
	mat4 jointMatrix[MAX_NUM_JOINTS];\n\
	float jointCount;\n\
} node;\n\
\n\
layout(location = 0) out vec3 outWorldPos;\n\
layout(location = 1) out vec3 outNormal;\n\
layout(location = 2) out vec2 outUV0;\n\
layout(location = 3) out vec2 outUV1;\n\
\n\
out gl_PerVertex\n\
{\n\
	vec4 gl_Position;\n\
};\n\
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
		locPos = ubo.model * node.matrix * skinMat * vec4(inPos, 1.0);\n\
		outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix * skinMat))) * inNormal);\n\
	}\n\
	else {\n\
		locPos = ubo.model * node.matrix * vec4(inPos, 1.0);\n\
		outNormal = normalize(transpose(inverse(mat3(ubo.model * node.matrix))) * inNormal);\n\
	}\n\
	locPos.y = -locPos.y;\n\
	outWorldPos = locPos.xyz / locPos.w;\n\
	outUV0 = inUV0;\n\
	outUV1 = inUV1;\n\
	gl_Position = ubo.projection * ubo.view * vec4(outWorldPos, 1.0);\n\
}";

// https://github.com/KhronosGroup/glTF-WebGL-PBR
		const char* pbrFragmentShader = "#version 450\n\
layout(location = 0) in vec3 inWorldPos;layout(location = 1) in vec3 inNormal;layout(location = 2) in vec2 inUV0;layout(location = 3) in vec2 inUV1;\n\
layout(set = 0, binding = 0) uniform UBO {mat4 projection;mat4 model;mat4 view;vec3 camPos; } ubo;\n\
layout(set = 0, binding = 1) uniform UBOParams {vec4 lightDir;float exposure;float gamma;float prefilteredCubeMipLevels;\n\
float scaleIBLAmbient;float debugViewInputs;float debugViewEquation;} uboParams;\n\
layout(set = 0, binding = 2) uniform samplerCube samplerIrradiance;layout(set = 0, binding = 3) uniform samplerCube prefilteredMap;\n\
layout(set = 0, binding = 4) uniform sampler2D samplerBRDFLUT;layout(set = 1, binding = 0) uniform sampler2D colorMap;\n\layout(set = 1, binding = 1) uniform sampler2D physicalDescriptorMap;\n\
layout(set = 1, binding = 2) uniform sampler2D normalMap;layout(set = 1, binding = 3) uniform sampler2D aoMap;layout(set = 1, binding = 4) uniform sampler2D emissiveMap;\n\
layout(push_constant) uniform Material {vec4 baseColorFactor;vec4 emissiveFactor;vec4 diffuseFactor;vec4 specularFactor;float workflow;\n\
int baseColorTextureSet;int physicalDescriptorTextureSet;int normalTextureSet;int occlusionTextureSet;int emissiveTextureSet;float metallicFactor;\n\
float roughnessFactor;float alphaMask;float alphaMaskCutoff;} material;\n\
layout(location = 0) out vec4 outColor;\n\
struct PBRInfo{float NdotL;float NdotV;float NdotH;float LdotH;float VdotH;float perceptualRoughness;float metalness;\n\
vec3 reflectance0;   vec3 reflectance90;float alphaRoughness;vec3 diffuseColor;vec3 specularColor;};const float M_PI = 3.141592653589793;\n\
const float c_MinRoughness = 0.04;const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;#define MANUAL_SRGB 1\n\
vec3 Uncharted2Tonemap(vec3 color){float A = 0.15;float B = 0.50;float C = 0.10;float D = 0.20;float E = 0.02;float F = 0.30;float W = 11.2;\n\
return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;}\n\
vec4 tonemap(vec4 color){vec3 outcol = Uncharted2Tonemap(color.rgb * uboParams.exposure);outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));\n\
return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);}\n\
vec4 SRGBtoLINEAR(vec4 srgbIn){\n\
#ifdef MANUAL_SRGB\n\
#ifdef SRGB_FAST_APPROXIMATION\n\
	vec3 linOut = pow(srgbIn.xyz, vec3(2.2));\n\
#else\n\
	vec3 bLess = step(vec3(0.04045), srgbIn.xyz);\n\
	vec3 linOut = mix(srgbIn.xyz / vec3(12.92), pow((srgbIn.xyz + vec3(0.055)) / vec3(1.055), vec3(2.4)), bLess);\n\
#endif\n\
	return vec4(linOut, srgbIn.w);\n\
#else\n\
	return srgbIn;\n\
#endif\n\
}\n\
vec3 getNormal(){vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0 - 1.0;\n\
vec3 q1 = dFdx(inWorldPos);vec3 q2 = dFdy(inWorldPos);vec2 st1 = dFdx(inUV0);vec2 st2 = dFdy(inUV0);vec3 N = normalize(inNormal);vec3 T = normalize(q1 * st2.t - q2 * st1.t);\n\
vec3 B = -normalize(cross(N, T));mat3 TBN = mat3(T, B, N);return normalize(TBN * tangentNormal);}\n\
vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection){\n\
float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0 - pbrInputs.perceptualRoughness))).rgb;\n\
vec3 diffuseLight = SRGBtoLINEAR(tonemap(texture(samplerIrradiance, n))).rgb;vec3 specularLight = SRGBtoLINEAR(tonemap(textureLod(prefilteredMap, reflection, lod))).rgb;\n\
vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);diffuse *= uboParams.scaleIBLAmbient;\n\
specular *= uboParams.scaleIBLAmbient;return diffuse + specular;}\n\
vec3 diffuse(PBRInfo pbrInputs){return pbrInputs.diffuseColor / M_PI;}\n\
vec3 specularReflection(PBRInfo pbrInputs){return pbrInputs.reflectance0 + (pbrInputs.reflectance90 - pbrInputs.reflectance0) * pow(clamp(1.0 - pbrInputs.VdotH, 0.0, 1.0), 5.0);}\n\
float geometricOcclusion(PBRInfo pbrInputs){float NdotL = pbrInputs.NdotL;float NdotV = pbrInputs.NdotV;float r = pbrInputs.alphaRoughness;\n\
float attenuationL = 2.0 * NdotL / (NdotL + sqrt(r * r + (1.0 - r * r) * (NdotL * NdotL)));float attenuationV = 2.0 * NdotV / (NdotV + sqrt(r * r + (1.0 - r * r) * (NdotV * NdotV)));\n\
return attenuationL * attenuationV;}\n\
float microfacetDistribution(PBRInfo pbrInputs){float roughnessSq = pbrInputs.alphaRoughness * pbrInputs.alphaRoughness;\n\
float f = (pbrInputs.NdotH * roughnessSq - pbrInputs.NdotH) * pbrInputs.NdotH + 1.0;return roughnessSq / (M_PI * f * f);}\n\
float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {float perceivedDiffuse = sqrt(0.299 * diffuse.r * diffuse.r + 0.587 * diffuse.g * diffuse.g + 0.114 * diffuse.b * diffuse.b);\n\
float perceivedSpecular = sqrt(0.299 * specular.r * specular.r + 0.587 * specular.g * specular.g + 0.114 * specular.b * specular.b);if (perceivedSpecular < c_MinRoughness) {return 0.0;}\n\
float a = c_MinRoughness;float b = perceivedDiffuse * (1.0 - maxSpecular) / (1.0 - c_MinRoughness) + perceivedSpecular - 2.0 * c_MinRoughness;float c = c_MinRoughness - perceivedSpecular;\n\
float D = max(b * b - 4.0 * a * c, 0.0);return clamp((-b + sqrt(D)) / (2.0 * a), 0.0, 1.0);}\n\
void main(){float perceptualRoughness;float metallic;vec3 diffuseColor;vec4 baseColor;vec3 f0 = vec3(0.04);if (material.alphaMask == 1.0f) {\n\
if (material.baseColorTextureSet > -1) {baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;}\n\
else {baseColor = material.baseColorFactor;}if (baseColor.a < material.alphaMaskCutoff) {discard;}}\n\
if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {perceptualRoughness = material.roughnessFactor;metallic = material.metallicFactor;if (material.physicalDescriptorTextureSet > -1) {\n\
vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);perceptualRoughness = mrSample.g * perceptualRoughness;metallic = mrSample.b * metallic;\n\
}else {perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);metallic = clamp(metallic, 0.0, 1.0);}if (material.baseColorTextureSet > -1) {\n\
baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;}else {baseColor = material.baseColorFactor;\n\
}}if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {if (material.physicalDescriptorTextureSet > -1) {\n\
perceptualRoughness = 1.0 - texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;}else {perceptualRoughness = 0.0;}const float epsilon = 1e-6;\n\
vec4 diffuse = SRGBtoLINEAR(texture(colorMap, inUV0));vec3 specular = SRGBtoLINEAR(texture(physicalDescriptorMap, inUV0)).rgb;float maxSpecular = max(max(specular.r, specular.g), specular.b);\n\
metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);vec3 baseColorDiffusePart = diffuse.rgb * ((1.0 - maxSpecular) / (1 - c_MinRoughness) / max(1 - metallic, epsilon)) * material.diffuseFactor.rgb;\n\
vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1 - metallic) * (1 / max(metallic, epsilon))) * material.specularFactor.rgb;baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);\n\
}diffuseColor = baseColor.rgb * (vec3(1.0) - f0);diffuseColor *= 1.0 - metallic;float alphaRoughness = perceptualRoughness * perceptualRoughness;vec3 specularColor = mix(f0, baseColor.rgb, metallic);\n\
float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);float reflectance90 = clamp(reflectance * 25.0, 0.0, 1.0);vec3 specularEnvironmentR0 = specularColor.rgb;\n\
vec3 specularEnvironmentR90 = vec3(1.0, 1.0, 1.0) * reflectance90;vec3 n = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);vec3 v = normalize(ubo.camPos - inWorldPos);\n\
vec3 l = normalize(uboParams.lightDir.xyz);vec3 h = normalize(l + v);vec3 reflection = -normalize(reflect(v, n));reflection.y *= -1.0f;float NdotL = clamp(dot(n, l), 0.001, 1.0);\n\
float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);float NdotH = clamp(dot(n, h), 0.0, 1.0);float LdotH = clamp(dot(l, h), 0.0, 1.0);float VdotH = clamp(dot(v, h), 0.0, 1.0);\n\
PBRInfo pbrInputs = PBRInfo(NdotL,NdotV,NdotH,LdotH,VdotH,perceptualRoughness,metallic,specularEnvironmentR0,specularEnvironmentR90,alphaRoughness,diffuseColor,specularColor);\n\
vec3 F = specularReflection(pbrInputs);float G = geometricOcclusion(pbrInputs);float D = microfacetDistribution(pbrInputs);const vec3 u_LightColor = vec3(1.0);\n\
vec3 diffuseContrib = (1.0 - F) * diffuse(pbrInputs);vec3 specContrib = F * G * D / (4.0 * NdotL * NdotV);vec3 color = NdotL * u_LightColor * (diffuseContrib + specContrib);color += getIBLContribution(pbrInputs, n, reflection);\n\
const float u_OcclusionStrength = 1.0f;if (material.occlusionTextureSet > -1) {float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;\n\
color = mix(color, color * ao, u_OcclusionStrength);}const float u_EmissiveFactor = 1.0f;if (material.emissiveTextureSet > -1) {vec3 emissive = SRGBtoLINEAR(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb * u_EmissiveFactor;color += emissive;\n\
}outColor = vec4(color, baseColor.a);if (uboParams.debugViewInputs > 0.0) {int index = int(uboParams.debugViewInputs);switch (index) {case 1:\n\
outColor.rgba = material.baseColorTextureSet > -1 ? texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) : vec4(1.0f);break;case 2:outColor.rgb = (material.normalTextureSet > -1) ? texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rgb : normalize(inNormal);\n\
break;case 3:outColor.rgb = (material.occlusionTextureSet > -1) ? texture(aoMap, material.occlusionTextureSet == 0 ? inUV0 : inUV1).rrr : vec3(0.0f);break;case 4:\n\
outColor.rgb = (material.emissiveTextureSet > -1) ? texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb : vec3(0.0f);break;case 5:outColor.rgb = texture(physicalDescriptorMap, inUV0).bbb;\n\
break;case 6:outColor.rgb = texture(physicalDescriptorMap, inUV0).ggg;break;}outColor = SRGBtoLINEAR(outColor);}if (uboParams.debugViewEquation > 0.0) {int index = int(uboParams.debugViewEquation);\n\
switch (index) {case 1:outColor.rgb = diffuseContrib;break;case 2:outColor.rgb = F;break;case 3:outColor.rgb = vec3(G);break;case 4:outColor.rgb = vec3(D);break;case 5:\n\
outColor.rgb = specContrib;break;}}}";


struct OpenGlPipelineObjects
{
	GLuint vao;
	GLuint shaderProgram;
}g_pipeline;
void InitializePbrPipeline()
{
	g_pipeline.shaderProgram = CreateProgram(pbrVertexShader, pbrFragmentShader);
	GLuint blockIdx = glGetUniformBlockIndex(g_pipeline.shaderProgram, "UBONode");
	glGenVertexArrays(1, &g_pipeline.vao);
	glBindVertexArray(g_pipeline.vao);


	

	glEnableVertexAttribArray(0);
	glEnableVertexAttribArray(1);
	glEnableVertexAttribArray(2);
	glEnableVertexAttribArray(3);
	glEnableVertexAttribArray(4);
	glEnableVertexAttribArray(5);

	glVertexAttribPointer(0, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)0);
	glVertexAttribPointer(1, 3, GL_FLOAT, GL_FALSE, 3 * sizeof(float), (const void*)(3 * sizeof(float)));
	glVertexAttribPointer(2, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)(6 * sizeof(float)));
	glVertexAttribPointer(3, 2, GL_FLOAT, GL_FALSE, 2 * sizeof(float), (const void*)(8 * sizeof(float)));
	glVertexAttribPointer(4, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(10 * sizeof(float)));
	glVertexAttribPointer(5, 4, GL_FLOAT, GL_FALSE, 4 * sizeof(float), (const void*)(14 * sizeof(float)));

	glBindVertexArray(0);
}


void DrawNode(InternalPBR* realObj, Node* node)
{	
	if (node->mesh) {
		for (Primitive* primitive : node->mesh->primitives)
		{
			glDrawElements(GL_TRIANGLES, primitive->indexCount, GL_UNSIGNED_INT, (const void*)(primitive->firstIndex * sizeof(uint32_t)));
		}
	}
	for (auto& child : node->children) {
		DrawNode(realObj, child);
	}
}
void DrawPBRModel(void* internalObj)
{
	InternalPBR* realObj = (InternalPBR*)internalObj;
	glBindVertexArray(g_pipeline.vao);
	glBindBuffer(GL_ARRAY_BUFFER, realObj->vertexBuffer);
	glBindBuffer(GL_ELEMENT_ARRAY_BUFFER, realObj->indices.indexBuffer);

	


}