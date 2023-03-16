#include "shader.h"
#include "GLCompat.h"
#include "logging.h"

static unsigned int _CreateProgram(const char* vtx, const char* frg) {
    unsigned int resultProgram = 0;
    unsigned int vertexShader = 0;
    unsigned int fragmentShader = 0;
    int success;
    char infoLog[512];
    vertexShader = glCreateShader(GL_VERTEX_SHADER);
    glShaderSource(vertexShader, 1, &vtx, NULL);
    glCompileShader(vertexShader);
    glGetShaderiv(vertexShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(vertexShader, 512, NULL, infoLog);
        LOG("[ERR]: Failed to Compile Vtx Shader %s\n", infoLog);
        glDeleteShader(vertexShader);
        return 0;
    }
    fragmentShader = glCreateShader(GL_FRAGMENT_SHADER);
    glShaderSource(fragmentShader, 1, &frg, NULL);
    glCompileShader(fragmentShader);
    glGetShaderiv(fragmentShader, GL_COMPILE_STATUS, &success);
    if(!success) {
        glGetShaderInfoLog(fragmentShader, 512, NULL, infoLog);
        LOG("[ERR]: Failed to Compile Frg Shader %s\n", infoLog);
        glDeleteShader(vertexShader); glDeleteShader(fragmentShader);
        return 0;
    }
    resultProgram = glCreateProgram();
    glAttachShader(resultProgram, vertexShader);
    glAttachShader(resultProgram, fragmentShader);
    glLinkProgram(resultProgram);
    glGetProgramiv(resultProgram, GL_LINK_STATUS, &success);
    glDeleteShader(vertexShader); glDeleteShader(fragmentShader);
    if(!success) {
        glGetProgramInfoLog(resultProgram, 512, NULL, infoLog);
        LOG("[ERR]: Failed to Link Program %s\n", infoLog);
        glDeleteProgram(fragmentShader);
        return 0;
    }
    return resultProgram;
}
static const char* REFL_VTX = "\n"
"#version 300 es\n"
"vec2 pos[6] = vec2[6](\n"
"    vec2(-0.5f, -0.5f),\n"
"    vec2( 0.5f, -0.5f),\n"
"    vec2( 0.5f,  0.5f),\n"
"    vec2( 0.5f,  0.5f),\n"
"    vec2(-0.5f,  0.5f),\n"
"    vec2(-0.5f, -0.5f)\n"
"    \n"
");\n"
"uniform mat4 projection;\n"
"uniform mat4 view;\n"
"uniform mat4 model;\n"
"uniform vec4 clipPlane;\n"
"out vec4 clipSpace;\n"
"out vec2 texCoord;\n"
"out vec4 fragPosWorld;\n"
"out vec3 normal;\n"
"out float clipDist;\n"
"void main() {\n"
"    vec4 worldPos = model * vec4(pos[gl_VertexID], 0.0f, 1.0f);\n"
"    normal = (model * vec4(0.0f, 0.0f, 1.0f, 0.0f)).xyz;\n"
"    fragPosWorld = worldPos;\n"
"    clipSpace = projection * view * worldPos;\n"
"    gl_Position = clipSpace;\n"
"    clipDist = dot(worldPos, clipPlane);\n"
"    texCoord = pos[gl_VertexID] + vec2(0.5f);\n"
"}\n";
static const char* AMBIENT_OCCLUSION_VTX = "\n"
"#version 300 es\n"
"vec2 pos[6] = vec2[6](\n"
"	vec2(-1.0f, -1.0f),\n"
"	vec2( 1.0f, -1.0f),\n"
"	vec2( 1.0f,  1.0f),\n"
"	vec2( 1.0f,  1.0f),\n"
"	vec2(-1.0f,  1.0f),\n"
"	vec2(-1.0f, -1.0f)\n"
");\n"
"out vec2 TexCoord;\n"
"void main()\n"
"{\n"
"    vec2 Position = pos[gl_VertexID];\n"
"	gl_Position = vec4(Position, 0.0f, 1.0);\n"
"    TexCoord = (Position.xy + vec2(1.0)) / 2.0;\n"
"}\n";
static const char* BLOOM_VTX = "#version 300 es\n"
"out vec2 UV;\n"
"out vec2 pos;\n"
"void main()\n"
"{\n"
"	UV = vec2((gl_VertexID << 1) & 2, gl_VertexID & 2);\n"
"    pos = UV * 2.0f - 1.0f;\n"
"	gl_Position = vec4(pos, 0.0f, 1.0f);\n"
"}\n";
static const char* PBR_VTX = "#version 300 es\n"
"\n"
"layout(location = 0) in vec3 inPos;\n"
"layout(location = 1) in vec3 inVNormal;\n"
"layout(location = 2) in vec2 inVUV0;\n"
"layout(location = 3) in vec2 inVUV1;\n"
"layout(location = 4) in vec4 inJoint0;\n"
"layout(location = 5) in vec4 inWeight0;\n"
"\n"
"uniform UBO\n"
"{\n"
"	mat4 projection;\n"
"	mat4 view;\n"
"	vec3 camPos;\n"
"} ubo;\n"
"uniform mat4 model;\n"
"uniform vec4 clipPlane;\n"
"\n"
"#define MAX_NUM_JOINTS 128\n"
"\n"
"uniform UBONode {\n"
"	mat4 matrix;\n"
"	mat4 jointMatrix[MAX_NUM_JOINTS];\n"
"	float jointCount;\n"
"} node;\n"
"\n"
"out vec3 inWorldPos;\n"
"out vec3 inNormal;\n"
"out vec2 inUV0;\n"
"out vec2 inUV1;\n"
"out float clipDist;\n"
"\n"
"void main()\n"
"{\n"
"	vec4 locPos;\n"
"	if (node.jointCount > 0.0) {\n"
"		// Mesh is skinned\n"
"		mat4 skinMat =\n"
"			inWeight0.x * node.jointMatrix[int(inJoint0.x)] +\n"
"			inWeight0.y * node.jointMatrix[int(inJoint0.y)] +\n"
"			inWeight0.z * node.jointMatrix[int(inJoint0.z)] +\n"
"			inWeight0.w * node.jointMatrix[int(inJoint0.w)];\n"
"	\n"
"		locPos = model * node.matrix * skinMat * vec4(inPos, 1.0);\n"
"		inNormal = normalize(transpose(inverse(mat3(model * node.matrix * skinMat))) * inVNormal);\n"
"	}\n"
"	else {\n"
"		locPos = model * node.matrix * vec4(inPos, 1.0);\n"
"		inNormal = normalize(transpose(inverse(mat3(model * node.matrix))) * inVNormal);\n"
"	}\n"
"	inWorldPos = locPos.xyz / locPos.w;\n"
"	inUV0 = inVUV0;\n"
"	inUV1 = inVUV1;\n"
"	clipDist = dot(vec4(inWorldPos, 1.0f), clipPlane);\n"
"	gl_Position = ubo.projection * ubo.view * vec4(inWorldPos, 1.0);\n"
"}\n";
static const char* CUBEMAP_VTX = "#version 300 es\n"
"layout (location = 0) in vec3  aPos;\n"
"out vec3 TexCoords;\n"
"\n"
"uniform mat4 viewProj;\n"
"\n"
"void main(){\n"
"	TexCoords = aPos;\n"
"	vec4 pos = viewProj * vec4(aPos, 0.0);\n"
"	gl_Position = pos.xyww;\n"
"}\n";
static const char* S3D_VTX = "#version 300 es\n"
"\n"
"layout(location = 0) in vec3 pos;\n"
"layout(location = 1) in vec2 texPos;\n"
"layout(location = 2) in vec4 color;\n"
"uniform mat4 projection;\n"
"uniform mat4 model;\n"
"uniform mat4 view;\n"
"uniform vec4 clipPlane;\n"
"out vec2 tPos;\n"
"out vec4 col;\n"
"out float clipDist;\n"
"void main(){\n"
"	vec4 pos = model * vec4(pos, 1.0f);\n"
"	gl_Position = projection * view * pos;\n"
"	tPos = texPos;\n"
"	col = color;\n"
"	clipDist = dot(pos, clipPlane);\n"
"}\n";
static const char* UI_VTX = "#version 300 es\n"
"layout(location = 0) in vec2 pos;\n"
"layout(location = 1) in vec2 texPos;\n"
"layout(location = 2) in vec4 color;\n"
"out vec2 tPos;\n"
"out vec4 col;\n"
"void main(){\n"
"	gl_Position = vec4(pos, 0.0f, 1.0f);\n"
"	tPos = texPos;\n"
"	col = color;\n"
"}\n";
static const char* REFL_FRG = "#version 300 es\n"
"precision highp float;\n"
"precision highp sampler2DShadow;\n"
"#define MAX_NUM_LIGHTS 8\n"
"#define MAX_NUM_SHADOW_MAP_CASCADE_COUNT 4\n"
"struct LightMapper\n"
"{\n"
"	mat4 viewProj;\n"
"	vec2 start;\n"
"	vec2 end;\n"
"};\n"
"struct CubemapLightMapperData\n"
"{\n"
"	vec4 startEnd[6];\n"
"};\n"
"struct PointLight\n"
"{\n"
"	vec3 pos;\n"
"	float constant;\n"
"	float linear;\n"
"	float quadratic;\n"
"	vec3 ambient;\n"
"	vec3 diffuse;\n"
"	vec3 specular;\n"
"	CubemapLightMapperData mapper;\n"
"	bool hasShadow;\n"
"};\n"
"struct DirectionalLight\n"
"{\n"
"	vec3 dir;\n"
"	vec3 ambient;\n"
"	vec3 diffuse;\n"
"	vec3 specular;\n"
"	LightMapper mapper[MAX_NUM_SHADOW_MAP_CASCADE_COUNT];\n"
"	vec4 cascadeSplits;\n"
"	int numCascades;\n"
"	bool hasShadow;\n"
"};\n"
"layout (std140) uniform LightData\n"
"{\n"
"	PointLight pointLights[MAX_NUM_LIGHTS];\n"
"	DirectionalLight dirLights[MAX_NUM_LIGHTS];\n"
"	int numPointLights;\n"
"	int numDirLights;\n"
"}_lightData;\n"
"uniform sampler2DShadow shadowMap;\n"
"uniform sampler2D _my_internalAOMap;\n"
"uniform vec2 currentFBOSize;\n"
"float CalculateShadowValue(LightMapper mapper, vec4 fragWorldPos)\n"
"{\n"
"	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\n"
"	vec4 fragLightPos = mapper.viewProj * fragWorldPos;\n"
"	vec3 projCoords = fragLightPos.xyz / fragLightPos.w;\n"
"	projCoords = (projCoords * 0.5 + 0.5) * vec3(mapper.end - mapper.start, 1.0f) + vec3(mapper.start, 0.0f);\n"
"	float shadow = 0.0f;\n"
"	for(int x = -1; x <= 1; ++x)\n"
"	{\n"
"		for(int y = -1; y <= 1; ++y)\n"
"		{\n"
"			vec3 testPos = projCoords + vec3(ts.x * float(x), ts.y * float(y), 0.0f);\n"
"			if(testPos.x < mapper.start.x || testPos.y < mapper.start.y || testPos.x > mapper.end.x || testPos.y > mapper.end.y) shadow += 1.0f;\n"
"			else shadow += texture(shadowMap, testPos.xyz);\n"
"		}\n"
"	}\n"
"	shadow /= 9.0f;\n"
"	return shadow;\n"
"}\n"
"vec3 convert_xyz_to_cube_uv(vec3 xyz)\n"
"{\n"
"	vec3 indexUVout = vec3(0.0f);\n"
"	float absX = abs(xyz.x);\n"
"	float absY = abs(xyz.y);\n"
"	float absZ = abs(xyz.z);\n"
"	int isXPositive = xyz.x > 0.0f ? 1 : 0;\n"
"	int isYPositive = xyz.y > 0.0f ? 1 : 0;\n"
"	int isZPositive = xyz.z > 0.0f ? 1 : 0;\n"
"	float maxAxis = 0.0f;\n"
"	float uc = 0.0f;\n"
"	float vc = 0.0f;\n"
"	if (isXPositive != 0 && absX >= absY && absX >= absZ) {\n"
"		maxAxis = absX;\n"
"		uc = -xyz.z;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 0.0f;\n"
"	}\n"
"	if (isXPositive == 0 && absX >= absY && absX >= absZ) {\n"
"		maxAxis = absX;\n"
"		uc = xyz.z;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 1.0f;\n"
"	}\n"
"	if (isYPositive != 0 && absY >= absX && absY >= absZ) {\n"
"		maxAxis = absY;\n"
"		uc = xyz.x;\n"
"		vc = -xyz.z;\n"
"		indexUVout.x = 2.0f;\n"
"	}\n"
"	if (isYPositive == 0 && absY >= absX && absY >= absZ) {\n"
"		maxAxis = absY;\n"
"		uc = xyz.x;\n"
"		vc = xyz.z;\n"
"		indexUVout.x = 3.0f;\n"
"	}\n"
"	if (isZPositive != 0 && absZ >= absX && absZ >= absY) {\n"
"		maxAxis = absZ;\n"
"		uc = xyz.x;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 4.0f;\n"
"	}\n"
"	if (isZPositive == 0 && absZ >= absX && absZ >= absY) {\n"
"		maxAxis = absZ;\n"
"		uc = -xyz.x;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 5.0f;\n"
"	}\n"
"	indexUVout.y = 0.5f * (uc / maxAxis + 1.0f);\n"
"	indexUVout.z = 0.5f * (vc / maxAxis + 1.0f);\n"
"	return indexUVout;\n"
"}\n"
"float CalculateCubeShadowValue(CubemapLightMapperData mapper, vec3 fragToLight, float fragPosWorld)\n"
"{\n"
"	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\n"
"	vec3 indexUV = convert_xyz_to_cube_uv(fragToLight);\n"
"	return 1.0f;\n"
"}\n"
"vec3 CalculatePointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\n"
"{\n"
"	vec3 lightDir = normalize(light.pos - fragPos);\n"
"	float diff = max(dot(normal, lightDir), 0.0f);\n"
"	vec3 reflectDir = reflect(-lightDir, normal);\n"
"	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
"	float distance = length(light.pos - fragPos);\n"
"	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
"	vec3 ambient = light.ambient * matDiffuseCol;\n"
"	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\n"
"	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\n"
"	ambient *= attenuation;\n"
"	diffuse *= attenuation;\n"
"	specular *= attenuation;\n"
"	return (ambient + diffuse + specular);\n"
"}\n"
"vec3 CalculateDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\n"
"{\n"
"	vec3 lightDir = normalize(-light.dir);\n"
"	float diff = max(dot(normal, lightDir), 0.0f);\n"
"	vec3 reflectDir = reflect(-lightDir, normal);\n"
"	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);\n"
"	vec3 ambient = light.ambient * matDiffuseCol;\n"
"	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\n"
"	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\n"
"	return (ambient + diffuse + specular);\n"
"}\n"
"vec3 CalculateLightsColor(vec4 fragWorldPos, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\n"
"{\n"
"	vec3 result = vec3(0.0f);\n"
"	float occludedValue = texture(_my_internalAOMap, (gl_FragCoord.xy - vec2(0.5)) / currentFBOSize).r;\n"
"	matDiffuseCol *= occludedValue;\n"
"	for(int i = 0; i < _lightData.numDirLights; i++)\n"
"	{\n"
"		float shadow = 1.0f;\n"
"		if(_lightData.dirLights[i].hasShadow){\n"
"			int mapperIndex = 0;\n"
"			for(int j = _lightData.dirLights[i].numCascades-1; j >= 0; j--)\n"
"			{\n"
"				if(_lightData.dirLights[i].cascadeSplits[j] < gl_FragCoord.z)\n"
"				{\n"
"					mapperIndex = j + 1;\n"
"					break;\n"
"				}\n"
"			}\n"
"			//if(mapperIndex == 0) result += vec3(1.0f, 0.0f, 0.0f);\n"
"			//else if(mapperIndex == 1) result += vec3(0.0f, 1.0f, 0.0f);\n"
"			//else if(mapperIndex == 2) result += vec3(0.0f, 0.0f, 1.0f);\n"
"			//else if(mapperIndex == 3) result += vec3(1.0f, 1.0f, 0.0f);\n"
"			//else if(mapperIndex == 4) result += vec3(1.0f, 0.0f, 1.0f);\n"
"			if(mapperIndex < 4)\n"
"			{\n"
"				shadow = CalculateShadowValue(_lightData.dirLights[i].mapper[mapperIndex], fragWorldPos);\n"
"			}\n"
"		}\n"
"		result += CalculateDirectionalLightColor(_lightData.dirLights[i], normal, viewDir, matDiffuseCol, matSpecCol, shininess, shadow);\n"
"	}\n"
"	for(int i = 0; i < _lightData.numPointLights; i++)\n"
"	{\n"
"		result += CalculatePointLightColor(_lightData.pointLights[i], normal, fragWorldPos.xyz, viewDir, matDiffuseCol, matSpecCol, shininess, 1.0f);\n"
"	}\n"
"	return result;\n"
"} \n"
"\n"
"precision highp float;\n"
"uniform Material {\n"
"    vec4 tintColor;\n"
"    float moveFactor;\n"
"    float distortionFactor;\n"
"    int type;\n"
"} material;\n"
"in vec4 clipSpace;\n"
"in vec2 texCoord;\n"
"in vec4 fragPosWorld;\n"
"in vec3 normal;\n"
"in float clipDist;\n"
"uniform mat4 projection;\n"
"uniform sampler2D reflectTexture;\n"
"uniform sampler2D refractTexture;\n"
"uniform sampler2D dvdu;\n"
"out vec4 outCol;\n"
"void main() {\n"
"    if(clipDist < 0.0f) discard;\n"
"    vec2 refract = clipSpace.xy / clipSpace.w * 0.5f + 0.5f;\n"
"    vec2 reflect = vec2(refract.x -refract.y);\n"
"    outCol = texture(reflectTexture, reflect);\n"
"	vec3 diffuseColor = ((vec4(0.9f) * outCol + vec4(0.1f) * texture(refractTexture, texCoord)) * material.tintColor).xyz;\n"
"	vec3 col = CalculateLightsColor(fragPosWorld, normalize(normal), vec3(0.0f), diffuseColor, vec3(0.0f), 0.0f);\n"
"    outCol = vec4(col, 1.0f);\n"
"}\n";
static const char* AMBIENT_OCCLUSION_FRG = "\n"
"#version 300 es\n"
"precision highp float;\n"
"in vec2 TexCoord;\n"
"out float FragColor;\n"
"uniform vec2 screenResolution;\n"
"uniform sampler2D gDepthMap;\n"
"uniform sampler2D gRandTex;\n"
"vec3 NormalFromDepth(float depth, vec2 Coords)\n"
"{\n"
"    vec2 offset1 = vec2(0.0, 1.0 / screenResolution.y);\n"
"    vec2 offset2 = vec2(1.0 / screenResolution.x, 0.0);\n"
"    float depth1 = texture(gDepthMap, Coords + offset1).r;\n"
"    float depth2 = texture(gDepthMap, Coords + offset2).r;\n"
"    vec3 p1 = vec3(offset1, depth1-depth);\n"
"    vec3 p2 = vec3(offset2, depth2-depth);\n"
"    vec3 normal = cross(p1, p2);\n"
"    normal.z = -normal.z;\n"
"    return normalize(normal);\n"
"}\n"
"vec3 sample_sphere[16] = vec3[16](\n"
"vec3(0.5381, 0.1856, -0.4319), vec3(0.1379, 0.2486, 0.4430),\n"
"vec3(0.3371, 0.5679, -0.0057), vec3(-0.6999, -0.0451, -0.0019),\n"
"vec3(0.0689, -0.1598, -0.8547), vec3(0.0560, 0.0069, -0.1843),\n"
"vec3(-0.0146, 0.1402, 0.0762), vec3(0.0100, -0.1924, -0.0344),\n"
"vec3(-0.3577, -0.5301, -0.4358), vec3(-0.3169, 0.1063, 0.0158),\n"
"vec3(0.0103, -0.5869, 0.0046), vec3(-0.0897, -0.4940, 0.3287),\n"
"vec3(0.7119, -0.0154, -0.0918), vec3(-0.0533, 0.0596, -0.5411),\n"
"vec3(0.0352, -0.0631, 0.5460), vec3(-0.4776, 0.2847, -0.0271)\n"
");\n"
"const float total_strength = 1.4;\n"
"const float base = 0.4;\n"
"const float area = 0.025;\n"
"const float falloff = 0.00001;\n"
"const float radius = 0.00525;\n"
"void main()\n"
"{\n"
"    float depth = texture(gDepthMap, TexCoord).r;\n"
"    vec3 random = normalize(texture(gRandTex, TexCoord * screenResolution / 4.0f).xyz);\n"
"    vec3 position = vec3(TexCoord, depth);\n"
"    vec3 normal = NormalFromDepth(depth, TexCoord);\n"
"    float radiusDepth = radius / depth;\n"
"    float occlusion = 0.0;\n"
"    for(int i = 0; i < 16; i++)\n"
"    {\n"
"        vec3 ray = radiusDepth * reflect(sample_sphere[i], random);\n"
"        vec3 hemiRay = position + sign(dot(ray, normal)) * ray;\n"
"        float occ_depth = texture(gDepthMap, clamp(hemiRay.xy, 0.0, 1.0)).r;\n"
"        float difference = depth - occ_depth;\n"
"        occlusion += step(falloff, difference) * (1.0 - smoothstep(falloff, area, difference));\n"
"    }\n"
"    float ao = 1.0 - total_strength * occlusion * (1.0 / 16.0);\n"
"    FragColor = clamp(ao + base, 0.0, 1.0);\n"
"}\n";
static const char* BLUR_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 UV;\n"
"in vec2 pos;\n"
"uniform sampler2D tex;\n"
"uniform float blurRadius;\n"
"uniform int axis;\n"
"uniform vec2 texUV;\n"
"out vec4 outCol;\n"
"void main()\n"
"{\n"
"    vec2 textureSize = vec2(textureSize(tex, 0));\n"
"    float x,y,rr=blurRadius*blurRadius,d,w,w0;\n"
"    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos) * texUV;\n"
"    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\n"
"    w0 = 0.5135 / pow(blurRadius, 0.96);\n"
"    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { \n"
"    w = w0 * exp((-x * x) / (2.0 * rr)); col += texture(tex, p) * w;\n"
" }\n"
"    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { \n"
"    w = w0 * exp((-y * y) / (2.0 * rr)); col += texture(tex, p) * w; \n"
"}\n"
"    outCol = col;\n"
"}\n";
static const char* BLOOM_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 UV;\n"
"in vec2 pos;\n"
"uniform sampler2D tex;\n"
"uniform float blurRadius;\n"
"uniform int axis;\n"
"uniform float intensity;\n"
"uniform float mipLevel;\n"
"out vec4 outCol;\n"
"void main()\n"
"{\n"
"    vec2 textureSize = vec2(textureSize(tex, int(mipLevel)));\n"
"    float x,y,rr=blurRadius*blurRadius,d,w,w0;\n"
"    vec2 p = 0.5 * (vec2(1.0, 1.0) + pos);\n"
"    vec4 col = vec4(0.0, 0.0, 0.0, 0.0);\n"
"    w0 = 0.5135 / pow(blurRadius, 0.96);\n"
"    if (axis == 0) for (d = 1.0 / textureSize.x, x = -blurRadius, p.x += x * d; x <= blurRadius; x++, p.x += d) { w = w0 * exp((-x * x) / (2.0 * rr));\n"
"            vec3 addCol = textureLod(tex, p, mipLevel).rgb;\n"
"            vec3 remCol = vec3(1.0f, 1.0f, 1.0f) * intensity;\n"
"            addCol = max(addCol - remCol, vec3(0.0f));\n"
"            col += vec4(addCol, 0.0f) * w;\n"
"        }\n"
"    if (axis == 1) for (d = 1.0 / textureSize.y, y = -blurRadius, p.y += y * d; y <= blurRadius; y++, p.y += d) { w = w0 * exp((-y * y) / (2.0 * rr));\n"
"            vec3 addCol = textureLod(tex, p, mipLevel).rgb;\n"
"            vec3 remCol = vec3(1.0f, 1.0f, 1.0f) * intensity;\n"
"            addCol = max(addCol - remCol, vec3(0.0f));\n"
"            col += vec4(addCol, 0.0f) * w;\n"
"        }\n"
"    outCol = col;\n"
"}\n";
static const char* COPY_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 UV;\n"
"in vec2 pos;\n"
"uniform sampler2D tex;\n"
"uniform float mipLevel;\n"
"out vec4 outCol;\n"
"void main()\n"
"{\n"
"    outCol = textureLod(tex, UV, mipLevel);\n"
"}\n";
static const char* UPSAMPLING_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 UV;\n"
"in vec2 pos;\n"
"uniform sampler2D tex;\n"
"uniform float mipLevel;\n"
"out vec4 outCol;\n"
"void main()\n"
"{\n"
"    vec2 ts = vec2(1.0f) / vec2(textureSize(tex, int(mipLevel)));\n"
"    vec3 c1 = textureLod(tex, UV + vec2(-ts.x, -ts.y), mipLevel).rgb;\n"
"    vec3 c2 = 2.0f * textureLod(tex, UV + vec2(0.0f, -ts.y), mipLevel).rgb;\n"
"    vec3 c3 = textureLod(tex, UV + vec2(ts.x, -ts.y), mipLevel).rgb;\n"
"    vec3 c4 = 2.0f * textureLod(tex, UV + vec2(-ts.x, 0.0f), mipLevel).rgb;\n"
"    vec3 c5 = 4.0f * textureLod(tex, UV + vec2(0.0f, 0.0f), mipLevel).rgb;\n"
"    vec3 c6 = 2.0f * textureLod(tex, UV + vec2(ts.x, 0.0f), mipLevel).rgb;\n"
"    vec3 c7 = textureLod(tex, UV + vec2(-ts.x, ts.y), mipLevel).rgb;\n"
"    vec3 c8 = 2.0f * textureLod(tex, UV + vec2(0.0f, ts.y), mipLevel).rgb;\n"
"    vec3 c9 = textureLod(tex, UV + vec2(ts.x, ts.y), mipLevel).rgb;\n"
"    vec3 accum = 1.0f / 16.0f * (c1 + c2 + c3 + c4 + c5 + c6 + c7 + c8 + c9);\n"
"    outCol = vec4(accum, 1.0f);\n"
"}\n";
static const char* DUAL_COPY_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 UV;\n"
"in vec2 pos;\n"
"uniform sampler2D tex1;\n"
"uniform sampler2D tex2;\n"
"uniform float mipLevel1;\n"
"uniform float mipLevel2;\n"
"vec3 aces(vec3 x) {\n"
"    const float a = 2.51;\n"
"    const float b = 0.03;\n"
"    const float c = 2.43;\n"
"    const float d = 0.59;\n"
"    const float e = 0.14;\n"
"    return clamp((x * (a * x + b)) / (x * (c * x + d) + e), 0.0, 1.0);\n"
"}\n"
"vec3 filmic(vec3 x) {\n"
"    vec3 X = max(vec3(0.0), x - 0.004);\n"
"    vec3 result = (X * (6.2 * X + 0.5)) / (X * (6.2 * X + 1.7) + 0.06);\n"
"    return pow(result, vec3(2.2));\n"
"}\n"
"out vec4 outCol;\n"
"void main()\n"
"{\n"
"    vec4 c = textureLod(tex1, UV, mipLevel1) + textureLod(tex2, UV, mipLevel2);\n"
"    outCol = c;\n"
"}\n";
static const char* PBR_FRG = "#version 300 es\n"
"precision highp float;\n"
"precision highp sampler2DShadow;\n"
"#define MAX_NUM_LIGHTS 8\n"
"#define MAX_NUM_SHADOW_MAP_CASCADE_COUNT 4\n"
"struct LightMapper\n"
"{\n"
"	mat4 viewProj;\n"
"	vec2 start;\n"
"	vec2 end;\n"
"};\n"
"struct CubemapLightMapperData\n"
"{\n"
"	vec4 startEnd[6];\n"
"};\n"
"struct PointLight\n"
"{\n"
"	vec3 pos;\n"
"	float constant;\n"
"	float linear;\n"
"	float quadratic;\n"
"	vec3 ambient;\n"
"	vec3 diffuse;\n"
"	vec3 specular;\n"
"	CubemapLightMapperData mapper;\n"
"	bool hasShadow;\n"
"};\n"
"struct DirectionalLight\n"
"{\n"
"	vec3 dir;\n"
"	vec3 ambient;\n"
"	vec3 diffuse;\n"
"	vec3 specular;\n"
"	LightMapper mapper[MAX_NUM_SHADOW_MAP_CASCADE_COUNT];\n"
"	vec4 cascadeSplits;\n"
"	int numCascades;\n"
"	bool hasShadow;\n"
"};\n"
"layout (std140) uniform LightData\n"
"{\n"
"	PointLight pointLights[MAX_NUM_LIGHTS];\n"
"	DirectionalLight dirLights[MAX_NUM_LIGHTS];\n"
"	int numPointLights;\n"
"	int numDirLights;\n"
"}_lightData;\n"
"uniform sampler2DShadow shadowMap;\n"
"uniform sampler2D _my_internalAOMap;\n"
"uniform vec2 currentFBOSize;\n"
"float CalculateShadowValue(LightMapper mapper, vec4 fragWorldPos)\n"
"{\n"
"	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\n"
"	vec4 fragLightPos = mapper.viewProj * fragWorldPos;\n"
"	vec3 projCoords = fragLightPos.xyz / fragLightPos.w;\n"
"	projCoords = (projCoords * 0.5 + 0.5) * vec3(mapper.end - mapper.start, 1.0f) + vec3(mapper.start, 0.0f);\n"
"	float shadow = 0.0f;\n"
"	for(int x = -1; x <= 1; ++x)\n"
"	{\n"
"		for(int y = -1; y <= 1; ++y)\n"
"		{\n"
"			vec3 testPos = projCoords + vec3(ts.x * float(x), ts.y * float(y), 0.0f);\n"
"			if(testPos.x < mapper.start.x || testPos.y < mapper.start.y || testPos.x > mapper.end.x || testPos.y > mapper.end.y) shadow += 1.0f;\n"
"			else shadow += texture(shadowMap, testPos.xyz);\n"
"		}\n"
"	}\n"
"	shadow /= 9.0f;\n"
"	return shadow;\n"
"}\n"
"vec3 convert_xyz_to_cube_uv(vec3 xyz)\n"
"{\n"
"	vec3 indexUVout = vec3(0.0f);\n"
"	float absX = abs(xyz.x);\n"
"	float absY = abs(xyz.y);\n"
"	float absZ = abs(xyz.z);\n"
"	int isXPositive = xyz.x > 0.0f ? 1 : 0;\n"
"	int isYPositive = xyz.y > 0.0f ? 1 : 0;\n"
"	int isZPositive = xyz.z > 0.0f ? 1 : 0;\n"
"	float maxAxis = 0.0f;\n"
"	float uc = 0.0f;\n"
"	float vc = 0.0f;\n"
"	if (isXPositive != 0 && absX >= absY && absX >= absZ) {\n"
"		maxAxis = absX;\n"
"		uc = -xyz.z;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 0.0f;\n"
"	}\n"
"	if (isXPositive == 0 && absX >= absY && absX >= absZ) {\n"
"		maxAxis = absX;\n"
"		uc = xyz.z;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 1.0f;\n"
"	}\n"
"	if (isYPositive != 0 && absY >= absX && absY >= absZ) {\n"
"		maxAxis = absY;\n"
"		uc = xyz.x;\n"
"		vc = -xyz.z;\n"
"		indexUVout.x = 2.0f;\n"
"	}\n"
"	if (isYPositive == 0 && absY >= absX && absY >= absZ) {\n"
"		maxAxis = absY;\n"
"		uc = xyz.x;\n"
"		vc = xyz.z;\n"
"		indexUVout.x = 3.0f;\n"
"	}\n"
"	if (isZPositive != 0 && absZ >= absX && absZ >= absY) {\n"
"		maxAxis = absZ;\n"
"		uc = xyz.x;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 4.0f;\n"
"	}\n"
"	if (isZPositive == 0 && absZ >= absX && absZ >= absY) {\n"
"		maxAxis = absZ;\n"
"		uc = -xyz.x;\n"
"		vc = xyz.y;\n"
"		indexUVout.x = 5.0f;\n"
"	}\n"
"	indexUVout.y = 0.5f * (uc / maxAxis + 1.0f);\n"
"	indexUVout.z = 0.5f * (vc / maxAxis + 1.0f);\n"
"	return indexUVout;\n"
"}\n"
"float CalculateCubeShadowValue(CubemapLightMapperData mapper, vec3 fragToLight, float fragPosWorld)\n"
"{\n"
"	vec2 ts = vec2(1.0f) / vec2(textureSize(shadowMap, 0));\n"
"	vec3 indexUV = convert_xyz_to_cube_uv(fragToLight);\n"
"	return 1.0f;\n"
"}\n"
"vec3 CalculatePointLightColor(PointLight light, vec3 normal, vec3 fragPos, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\n"
"{\n"
"	vec3 lightDir = normalize(light.pos - fragPos);\n"
"	float diff = max(dot(normal, lightDir), 0.0f);\n"
"	vec3 reflectDir = reflect(-lightDir, normal);\n"
"	float spec = pow(max(dot(viewDir, reflectDir), 0.0), shininess);\n"
"	float distance = length(light.pos - fragPos);\n"
"	float attenuation = 1.0f / (light.constant + light.linear * distance + light.quadratic * (distance * distance));\n"
"	vec3 ambient = light.ambient * matDiffuseCol;\n"
"	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\n"
"	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\n"
"	ambient *= attenuation;\n"
"	diffuse *= attenuation;\n"
"	specular *= attenuation;\n"
"	return (ambient + diffuse + specular);\n"
"}\n"
"vec3 CalculateDirectionalLightColor(DirectionalLight light, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess, float shadowValue)\n"
"{\n"
"	vec3 lightDir = normalize(-light.dir);\n"
"	float diff = max(dot(normal, lightDir), 0.0f);\n"
"	vec3 reflectDir = reflect(-lightDir, normal);\n"
"	float spec = pow(max(dot(viewDir, reflectDir), 0.0f), shininess);\n"
"	vec3 ambient = light.ambient * matDiffuseCol;\n"
"	vec3 diffuse = light.diffuse * diff * matDiffuseCol * shadowValue;\n"
"	vec3 specular = light.specular * spec * matSpecCol * shadowValue;\n"
"	return (ambient + diffuse + specular);\n"
"}\n"
"vec3 CalculateLightsColor(vec4 fragWorldPos, vec3 normal, vec3 viewDir, vec3 matDiffuseCol, vec3 matSpecCol, float shininess)\n"
"{\n"
"	vec3 result = vec3(0.0f);\n"
"	float occludedValue = texture(_my_internalAOMap, (gl_FragCoord.xy - vec2(0.5)) / currentFBOSize).r;\n"
"	matDiffuseCol *= occludedValue;\n"
"	for(int i = 0; i < _lightData.numDirLights; i++)\n"
"	{\n"
"		float shadow = 1.0f;\n"
"		if(_lightData.dirLights[i].hasShadow){\n"
"			int mapperIndex = 0;\n"
"			for(int j = _lightData.dirLights[i].numCascades-1; j >= 0; j--)\n"
"			{\n"
"				if(_lightData.dirLights[i].cascadeSplits[j] < gl_FragCoord.z)\n"
"				{\n"
"					mapperIndex = j + 1;\n"
"					break;\n"
"				}\n"
"			}\n"
"			//if(mapperIndex == 0) result += vec3(1.0f, 0.0f, 0.0f);\n"
"			//else if(mapperIndex == 1) result += vec3(0.0f, 1.0f, 0.0f);\n"
"			//else if(mapperIndex == 2) result += vec3(0.0f, 0.0f, 1.0f);\n"
"			//else if(mapperIndex == 3) result += vec3(1.0f, 1.0f, 0.0f);\n"
"			//else if(mapperIndex == 4) result += vec3(1.0f, 0.0f, 1.0f);\n"
"			if(mapperIndex < 4)\n"
"			{\n"
"				shadow = CalculateShadowValue(_lightData.dirLights[i].mapper[mapperIndex], fragWorldPos);\n"
"			}\n"
"		}\n"
"		result += CalculateDirectionalLightColor(_lightData.dirLights[i], normal, viewDir, matDiffuseCol, matSpecCol, shininess, shadow);\n"
"	}\n"
"	for(int i = 0; i < _lightData.numPointLights; i++)\n"
"	{\n"
"		result += CalculatePointLightColor(_lightData.pointLights[i], normal, fragWorldPos.xyz, viewDir, matDiffuseCol, matSpecCol, shininess, 1.0f);\n"
"	}\n"
"	return result;\n"
"} \n"
"\n"
"in vec3 inWorldPos;\n"
"in vec3 inNormal;\n"
"in vec2 inUV0;\n"
"in vec2 inUV1;\n"
"in float clipDist;\n"
"\n"
"\n"
"\n"
"uniform UBO {\n"
"	mat4 projection;\n"
"	mat4 view;\n"
"	vec3 camPos;\n"
"} ubo;\n"
"uniform mat4 model;\n"
"\n"
"uniform UBOParams {\n"
"	vec4 lightDir;\n"
"	float exposure;\n"
"	float gamma;\n"
"	float prefilteredCubeMipLevels;\n"
"	float scaleIBLAmbient;\n"
"	float debugViewInputs;\n"
"	float debugViewEquation;\n"
"} uboParams;\n"
"\n"
"uniform samplerCube samplerIrradiance;\n"
"uniform samplerCube prefilteredMap;\n"
"uniform sampler2D samplerBRDFLUT;\n"
"\n"
"\n"
"\n"
"uniform sampler2D colorMap;\n"
"uniform sampler2D physicalDescriptorMap;\n"
"uniform sampler2D normalMap;\n"
"uniform sampler2D aoMap;\n"
"uniform sampler2D emissiveMap;\n"
"\n"
"uniform Material {\n"
"	vec4 baseColorFactor;\n"
"	vec4 emissiveFactor;\n"
"	vec4 diffuseFactor;\n"
"	vec4 specularFactor;\n"
"	float workflow;\n"
"	int baseColorTextureSet;\n"
"	int physicalDescriptorTextureSet;\n"
"	int normalTextureSet;\n"
"	int occlusionTextureSet;\n"
"	int emissiveTextureSet;\n"
"	float metallicFactor;\n"
"	float roughnessFactor;\n"
"	float alphaMask;\n"
"	float alphaMaskCutoff;\n"
"} material;\n"
"\n"
"out vec4 outColor;\n"
"\n"
"\n"
"\n"
"\n"
"struct PBRInfo\n"
"{\n"
"	float NdotV;                  \n"
"	float perceptualRoughness;    \n"
"	float metalness;              \n"
"	vec3 reflectance0;            \n"
"	vec3 reflectance90;           \n"
"	float alphaRoughness;         \n"
"	vec3 diffuseColor;            \n"
"	vec3 specularColor;           \n"
"};\n"
"\n"
"const float M_PI = 3.141592653589793;\n"
"const float c_MinRoughness = 0.04;\n"
"\n"
"const float PBR_WORKFLOW_METALLIC_ROUGHNESS = 0.0;\n"
"const float PBR_WORKFLOW_SPECULAR_GLOSINESS = 1.0f;\n"
"\n"
"#define MANUAL_SRGB 0\n"
"\n"
"vec3 Uncharted2Tonemap(vec3 color)\n"
"{\n"
"	float A = 0.15;\n"
"	float B = 0.50;\n"
"	float C = 0.10;\n"
"	float D = 0.20;\n"
"	float E = 0.02;\n"
"	float F = 0.30;\n"
"	float W = 11.2;\n"
"	return ((color * (A * color + C * B) + D * E) / (color * (A * color + B) + D * F)) - E / F;\n"
"}\n"
"\n"
"vec4 tonemap(vec4 color)\n"
"{\n"
"	vec3 outcol = Uncharted2Tonemap(color.rgb * uboParams.exposure);\n"
"	outcol = outcol * (1.0f / Uncharted2Tonemap(vec3(11.2f)));\n"
"	return vec4(pow(outcol, vec3(1.0f / uboParams.gamma)), color.a);\n"
"}\n"
"\n"
"vec4 SRGBtoLINEAR(vec4 srgbIn)\n"
"{\n"
"#ifdef MANUAL_SRGB\n"
"#ifdef SRGB_FAST_APPROXIMATION\n"
"	vec3 linOut = pow(srgbIn.xyz, vec3(2.2f));\n"
"#else //SRGB_FAST_APPROXIMATION\n"
"	vec3 bLess = step(vec3(0.04045f), srgbIn.xyz);\n"
"	vec3 linOut = mix(srgbIn.xyz / vec3(12.92f), pow((srgbIn.xyz + vec3(0.055f)) / vec3(1.055f), vec3(2.4f)), bLess);\n"
"#endif //SRGB_FAST_APPROXIMATION\n"
"	return vec4(linOut, srgbIn.w);\n"
"#else //MANUAL_SRGB\n"
"	return srgbIn;\n"
"#endif //MANUAL_SRGB\n"
"}\n"
"\n"
"vec3 getNormal()\n"
"{\n"
"\n"
"	vec3 tangentNormal = texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).xyz * 2.0f - 1.0f;\n"
"\n"
"	vec3 q1 = dFdx(inWorldPos);\n"
"	vec3 q2 = dFdy(inWorldPos);\n"
"	vec2 st1 = dFdx(inUV0);\n"
"	vec2 st2 = dFdy(inUV0);\n"
"\n"
"	vec3 N = normalize(inNormal);\n"
"	vec3 T = normalize(q1 * st2.t - q2 * st1.t);\n"
"	vec3 B = -normalize(cross(N, T));\n"
"	mat3 TBN = mat3(T, B, N);\n"
"\n"
"	return normalize(TBN * tangentNormal);\n"
"}\n"
"\n"
"\n"
"vec3 getIBLContribution(PBRInfo pbrInputs, vec3 n, vec3 reflection)\n"
"{\n"
"	float lod = (pbrInputs.perceptualRoughness * uboParams.prefilteredCubeMipLevels);\n"
"	// retrieve a scale and bias to F0. See [1], Figure 3\n"
"	vec3 brdf = (texture(samplerBRDFLUT, vec2(pbrInputs.NdotV, 1.0f - pbrInputs.perceptualRoughness))).rgb;\n"
"	vec3 diffuseLight = SRGBtoLINEAR(tonemap(texture(samplerIrradiance, n))).rgb;\n"
"\n"
"	vec3 specularLight = SRGBtoLINEAR(tonemap(textureLod(prefilteredMap, reflection, lod))).rgb;\n"
"\n"
"	vec3 diffuse = diffuseLight * pbrInputs.diffuseColor;\n"
"	vec3 specular = specularLight * (pbrInputs.specularColor * brdf.x + brdf.y);\n"
"\n"
"	diffuse *= uboParams.scaleIBLAmbient;\n"
"	specular *= uboParams.scaleIBLAmbient;\n"
"\n"
"	return diffuse + specular;\n"
"}\n"
"\n"
"vec3 diffuse(PBRInfo pbrInputs)\n"
"{\n"
"	return pbrInputs.diffuseColor / M_PI;\n"
"}\n"
"\n"
"\n"
"float convertMetallic(vec3 diffuse, vec3 specular, float maxSpecular) {\n"
"	float perceivedDiffuse = sqrt(0.299f * diffuse.r * diffuse.r + 0.587f * diffuse.g * diffuse.g + 0.114f * diffuse.b * diffuse.b);\n"
"	float perceivedSpecular = sqrt(0.299f * specular.r * specular.r + 0.587f * specular.g * specular.g + 0.114f * specular.b * specular.b);\n"
"	if (perceivedSpecular < c_MinRoughness) {\n"
"		return 0.0;\n"
"	}\n"
"	float a = c_MinRoughness;\n"
"	float b = perceivedDiffuse * (1.0f - maxSpecular) / (1.0f - c_MinRoughness) + perceivedSpecular - 2.0f * c_MinRoughness;\n"
"	float c = c_MinRoughness - perceivedSpecular;\n"
"	float D = max(b * b - 4.0 * a * c, 0.0f);\n"
"	return clamp((-b + sqrt(D)) / (2.0f * a), 0.0f, 1.0f);\n"
"}\n"
"\n"
"void main()\n"
"{\n"
"	if(clipDist < 0.0f) discard;\n"
"	float perceptualRoughness;\n"
"	float metallic;\n"
"	vec3 diffuseColor;\n"
"	vec4 baseColor;\n"
"\n"
"	vec3 f0 = vec3(0.04f);\n"
"\n"
"	if (material.alphaMask == 1.0f) {\n"
"		if (material.baseColorTextureSet > -1) {\n"
"			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;\n"
"		}\n"
"		else {\n"
"			baseColor = material.baseColorFactor;\n"
"		}\n"
"		if (baseColor.a < material.alphaMaskCutoff) {\n"
"			discard;\n"
"		}\n"
"	}\n"
"\n"
"	if (material.workflow == PBR_WORKFLOW_METALLIC_ROUGHNESS) {\n"
"		perceptualRoughness = material.roughnessFactor;\n"
"		metallic = material.metallicFactor;\n"
"		if (material.physicalDescriptorTextureSet > -1) {\n"
"			vec4 mrSample = texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1);\n"
"			perceptualRoughness = mrSample.g * perceptualRoughness;\n"
"			metallic = mrSample.b * metallic;\n"
"		}\n"
"		else {\n"
"			perceptualRoughness = clamp(perceptualRoughness, c_MinRoughness, 1.0);\n"
"			metallic = clamp(metallic, 0.0, 1.0);\n"
"		}\n"
"		if (material.baseColorTextureSet > -1) {\n"
"			baseColor = SRGBtoLINEAR(texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1)) * material.baseColorFactor;\n"
"		}\n"
"		else {\n"
"			baseColor = material.baseColorFactor;\n"
"		}\n"
"	}\n"
"\n"
"	if (material.workflow == PBR_WORKFLOW_SPECULAR_GLOSINESS) {\n"
"		// Values from specular glossiness workflow are converted to metallic roughness\n"
"		if (material.physicalDescriptorTextureSet > -1) {\n"
"			perceptualRoughness = 1.0 - texture(physicalDescriptorMap, material.physicalDescriptorTextureSet == 0 ? inUV0 : inUV1).a;\n"
"		}\n"
"		else {\n"
"			perceptualRoughness = 0.0;\n"
"		}\n"
"\n"
"		const float epsilon = 1e-6;\n"
"\n"
"		vec4 diffuse = SRGBtoLINEAR(texture(colorMap, inUV0));\n"
"		vec3 specular = SRGBtoLINEAR(texture(physicalDescriptorMap, inUV0)).rgb;\n"
"\n"
"		float maxSpecular = max(max(specular.r, specular.g), specular.b);\n"
"\n"
"		// Convert metallic value from specular glossiness inputs\n"
"		metallic = convertMetallic(diffuse.rgb, specular, maxSpecular);\n"
"\n"
"		vec3 baseColorDiffusePart = diffuse.rgb * ((1.0f - maxSpecular) / (1.0f - c_MinRoughness) / max(1.0f - metallic, epsilon)) * material.diffuseFactor.rgb;\n"
"		vec3 baseColorSpecularPart = specular - (vec3(c_MinRoughness) * (1.0f - metallic) * (1.0f / max(metallic, epsilon))) * material.specularFactor.rgb;\n"
"		baseColor = vec4(mix(baseColorDiffusePart, baseColorSpecularPart, metallic * metallic), diffuse.a);\n"
"	}\n"
"\n"
"	diffuseColor = baseColor.rgb * (vec3(1.0f) - f0);\n"
"	diffuseColor *= 1.0f - metallic;\n"
"\n"
"	float alphaRoughness = perceptualRoughness * perceptualRoughness;\n"
"\n"
"	vec3 specularColor = mix(f0, baseColor.rgb, metallic);\n"
"\n"
"	float reflectance = max(max(specularColor.r, specularColor.g), specularColor.b);\n"
"\n"
"	float reflectance90 = clamp(reflectance * 25.0f, 0.0f, 1.0f);\n"
"	vec3 specularEnvironmentR0 = specularColor.rgb;\n"
"	vec3 specularEnvironmentR90 = vec3(1.0f, 1.0f, 1.0f) * reflectance90;\n"
"\n"
"	vec3 n = (material.normalTextureSet > -1) ? getNormal() : normalize(inNormal);\n"
"	vec3 v = normalize(ubo.camPos - inWorldPos);    // Vector from surface point to camera\n"
"	vec3 reflection = -normalize(reflect(v, n));\n"
"\n"
"	float NdotV = clamp(abs(dot(n, v)), 0.001, 1.0);\n"
"\n"
"	PBRInfo pbrInputs = PBRInfo(\n"
"		NdotV,\n"
"		perceptualRoughness,\n"
"		metallic,\n"
"		specularEnvironmentR0,\n"
"		specularEnvironmentR90,\n"
"		alphaRoughness,\n"
"		diffuseColor,\n"
"		specularColor\n"
"	);\n"
"\n"
"	// Calculate the shading terms for the microfacet specular shading model\n"
"	vec3 color = CalculateLightsColor(vec4(inWorldPos, 1.0f), n, v, diffuseColor, specularColor, reflectance90);\n"
"\n"
"	// Calculate lighting contribution from image based lighting source (IBL)\n"
"	color += getIBLContribution(pbrInputs, n, reflection);\n"
"\n"
"	const float u_OcclusionStrength = 1.0f;\n"
"	if (material.occlusionTextureSet > -1) {\n"
"		float ao = texture(aoMap, (material.occlusionTextureSet == 0 ? inUV0 : inUV1)).r;\n"
"		color = mix(color, color * ao, u_OcclusionStrength);\n"
"	}\n"
"\n"
"	const float u_EmissiveFactor = 1.0f;\n"
"	if (material.emissiveTextureSet > -1) {\n"
"		vec3 emissive = SRGBtoLINEAR(texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1)).rgb * u_EmissiveFactor;\n"
"		color += emissive;\n"
"	}\n"
"\n"
"	outColor = vec4(color, baseColor.a);\n"
"\n"
"	// Shader inputs debug visualization\n"
"	if (uboParams.debugViewInputs > 0.0) {\n"
"		int index = int(uboParams.debugViewInputs);\n"
"		switch (index) {\n"
"		case 1:\n"
"			outColor.rgba = material.baseColorTextureSet > -1 ? texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) : vec4(1.0f);\n"
"			break;\n"
"		case 2:\n"
"			outColor.rgb = (material.normalTextureSet > -1) ? texture(normalMap, material.normalTextureSet == 0 ? inUV0 : inUV1).rgb : normalize(inNormal);\n"
"			break;\n"
"		case 3:\n"
"			outColor.rgb = (material.occlusionTextureSet > -1) ? texture(aoMap, material.occlusionTextureSet == 0 ? inUV0 : inUV1).rrr : vec3(0.0f);\n"
"			break;\n"
"		case 4:\n"
"			outColor.rgb = (material.emissiveTextureSet > -1) ? texture(emissiveMap, material.emissiveTextureSet == 0 ? inUV0 : inUV1).rgb : vec3(0.0f);\n"
"			break;\n"
"		case 5:\n"
"			outColor.rgb = texture(physicalDescriptorMap, inUV0).bbb;\n"
"			break;\n"
"		case 6:\n"
"			outColor.rgb = texture(physicalDescriptorMap, inUV0).ggg;\n"
"			break;\n"
"		}\n"
"		outColor = SRGBtoLINEAR(outColor);\n"
"	}\n"
"}\n";
static const char* PBR_GEOM_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 inUV0;\n"
"in vec2 inUV1;\n"
"uniform Material {\n"
"	vec4 baseColorFactor;\n"
"	vec4 emissiveFactor;\n"
"	vec4 diffuseFactor;\n"
"	vec4 specularFactor;\n"
"	float workflow;\n"
"	int baseColorTextureSet;\n"
"	int physicalDescriptorTextureSet;\n"
"	int normalTextureSet;\n"
"	int occlusionTextureSet;\n"
"	int emissiveTextureSet;\n"
"	float metallicFactor;\n"
"	float roughnessFactor;\n"
"	float alphaMask;\n"
"	float alphaMaskCutoff;\n"
"} material;\n"
"uniform sampler2D colorMap;\n"
"void main()\n"
"{\n"
"	vec4 baseColor;\n"
"	if (material.alphaMask == 1.0f) {\n"
"		if (material.baseColorTextureSet > -1) {\n"
"			baseColor = texture(colorMap, material.baseColorTextureSet == 0 ? inUV0 : inUV1) * material.baseColorFactor;\n"
"		}\n"
"		else {\n"
"			baseColor = material.baseColorFactor;\n"
"		}\n"
"		if (baseColor.a < material.alphaMaskCutoff) {\n"
"			discard;\n"
"		}\n"
"	}\n"
"}\n";
static const char* EMPTY_FRG = "#version 300 es\n"
"precision highp float;\n"
"void main()\n"
"{\n"
"}\n";
static const char* CUBEMAP_FRG = "#version 300 es\n"
"precision highp float;\n"
"out vec4 FragColor;\n"
"\n"
"in vec3 TexCoords;\n"
"\n"
"uniform samplerCube skybox;\n"
"void main()\n"
"{\n"
"	FragColor = textureLod(skybox, TexCoords, 0.0);\n"
"}\n";
static const char* S3D_FRG = "#version 300 es\n"
"precision highp float;\n"
"\n"
"in vec2 tPos;\n"
"in vec4 col;\n"
"in float clipDist;\n"
"uniform sampler2D tex;\n"
"out vec4 outCol;\n"
"void main(){\n"
"	if(clipDist < 0.0f) discard;\n"
"	outCol = texture(tex, tPos).rgba * col;\n"
"}\n";
static const char* UI_FRG = "#version 300 es\n"
"precision highp float;\n"
"in vec2 tPos;\n"
"in vec4 col;\n"
"uniform sampler2D tex;\n"
"out vec4 outCol;\n"
"void main(){\n"
"	outCol = texture(tex, tPos).rgba * col;\n"
"	outCol = vec4(texture(tex, tPos).rgb, 1.0f) * col;\n"
"}\n";
void SH_Create(struct Shader* out) {
	memset(out, 0, sizeof(struct Shader));
	out->reflect = _CreateProgram(REFL_VTX, REFL_FRG);
	glUseProgram(out->reflect);
	out->_internal_uniforms.idx_reflect_projection = glGetUniformLocation(out->reflect, "projection");
	out->_internal_uniforms.idx_reflect_view = glGetUniformLocation(out->reflect, "view");
	out->_internal_uniforms.idx_reflect_model = glGetUniformLocation(out->reflect, "model");
	out->_internal_uniforms.idx_reflect_clipPlane = glGetUniformLocation(out->reflect, "clipPlane");
	out->_internal_uniforms.idx_reflect__lightData = glGetUniformBlockIndex(out->reflect, "_lightData");
	{
		unsigned int uniform = glGetUniformLocation(out->reflect, "shadowMap");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->reflect, "_my_internalAOMap");
		if(uniform != -1) glUniform1i(uniform, 1);
	}
	out->_internal_uniforms.idx_reflect_currentFBOSize = glGetUniformLocation(out->reflect, "currentFBOSize");
	out->_internal_uniforms.idx_reflect_material = glGetUniformBlockIndex(out->reflect, "material");
	{
		unsigned int uniform = glGetUniformLocation(out->reflect, "reflectTexture");
		if(uniform != -1) glUniform1i(uniform, 2);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->reflect, "refractTexture");
		if(uniform != -1) glUniform1i(uniform, 3);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->reflect, "dvdu");
		if(uniform != -1) glUniform1i(uniform, 4);
	}
	out->reflect_geom = _CreateProgram(REFL_VTX, EMPTY_FRG);
	glUseProgram(out->reflect_geom);
	out->_internal_uniforms.idx_reflect_geom_projection = glGetUniformLocation(out->reflect_geom, "projection");
	out->_internal_uniforms.idx_reflect_geom_view = glGetUniformLocation(out->reflect_geom, "view");
	out->_internal_uniforms.idx_reflect_geom_model = glGetUniformLocation(out->reflect_geom, "model");
	out->_internal_uniforms.idx_reflect_geom_clipPlane = glGetUniformLocation(out->reflect_geom, "clipPlane");
	out->ao = _CreateProgram(AMBIENT_OCCLUSION_VTX, AMBIENT_OCCLUSION_FRG);
	glUseProgram(out->ao);
	out->_internal_uniforms.idx_ao_screenResolution = glGetUniformLocation(out->ao, "screenResolution");
	{
		unsigned int uniform = glGetUniformLocation(out->ao, "gDepthMap");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->ao, "gRandTex");
		if(uniform != -1) glUniform1i(uniform, 1);
	}
	out->blur = _CreateProgram(BLOOM_VTX, BLUR_FRG);
	glUseProgram(out->blur);
	{
		unsigned int uniform = glGetUniformLocation(out->blur, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->_internal_uniforms.idx_blur_blurRadius = glGetUniformLocation(out->blur, "blurRadius");
	out->_internal_uniforms.idx_blur_axis = glGetUniformLocation(out->blur, "axis");
	out->_internal_uniforms.idx_blur_texUV = glGetUniformLocation(out->blur, "texUV");
	out->bloom = _CreateProgram(BLOOM_VTX, BLOOM_FRG);
	glUseProgram(out->bloom);
	{
		unsigned int uniform = glGetUniformLocation(out->bloom, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->_internal_uniforms.idx_bloom_blurRadius = glGetUniformLocation(out->bloom, "blurRadius");
	out->_internal_uniforms.idx_bloom_axis = glGetUniformLocation(out->bloom, "axis");
	out->_internal_uniforms.idx_bloom_intensity = glGetUniformLocation(out->bloom, "intensity");
	out->_internal_uniforms.idx_bloom_mipLevel = glGetUniformLocation(out->bloom, "mipLevel");
	out->copy = _CreateProgram(BLOOM_VTX, COPY_FRG);
	glUseProgram(out->copy);
	{
		unsigned int uniform = glGetUniformLocation(out->copy, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->_internal_uniforms.idx_copy_mipLevel = glGetUniformLocation(out->copy, "mipLevel");
	out->dual_copy = _CreateProgram(BLOOM_VTX, DUAL_COPY_FRG);
	glUseProgram(out->dual_copy);
	{
		unsigned int uniform = glGetUniformLocation(out->dual_copy, "tex1");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->dual_copy, "tex2");
		if(uniform != -1) glUniform1i(uniform, 1);
	}
	out->_internal_uniforms.idx_dual_copy_mipLevel1 = glGetUniformLocation(out->dual_copy, "mipLevel1");
	out->_internal_uniforms.idx_dual_copy_mipLevel2 = glGetUniformLocation(out->dual_copy, "mipLevel2");
	out->upsample = _CreateProgram(BLOOM_VTX, UPSAMPLING_FRG);
	glUseProgram(out->upsample);
	{
		unsigned int uniform = glGetUniformLocation(out->upsample, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->_internal_uniforms.idx_upsample_mipLevel = glGetUniformLocation(out->upsample, "mipLevel");
	out->cubemap = _CreateProgram(CUBEMAP_VTX, CUBEMAP_FRG);
	glUseProgram(out->cubemap);
	out->_internal_uniforms.idx_cubemap_viewProj = glGetUniformLocation(out->cubemap, "viewProj");
	{
		unsigned int uniform = glGetUniformLocation(out->cubemap, "skybox");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->pbr = _CreateProgram(PBR_VTX, PBR_FRG);
	glUseProgram(out->pbr);
	out->_internal_uniforms.idx_pbr_ubo = glGetUniformBlockIndex(out->pbr, "ubo");
	out->_internal_uniforms.idx_pbr_model = glGetUniformLocation(out->pbr, "model");
	out->_internal_uniforms.idx_pbr_clipPlane = glGetUniformLocation(out->pbr, "clipPlane");
	out->_internal_uniforms.idx_pbr_node = glGetUniformBlockIndex(out->pbr, "node");
	out->_internal_uniforms.idx_pbr__lightData = glGetUniformBlockIndex(out->pbr, "_lightData");
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "shadowMap");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "_my_internalAOMap");
		if(uniform != -1) glUniform1i(uniform, 1);
	}
	out->_internal_uniforms.idx_pbr_currentFBOSize = glGetUniformLocation(out->pbr, "currentFBOSize");
	out->_internal_uniforms.idx_pbr_uboParams = glGetUniformBlockIndex(out->pbr, "uboParams");
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "samplerIrradiance");
		if(uniform != -1) glUniform1i(uniform, 2);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "prefilteredMap");
		if(uniform != -1) glUniform1i(uniform, 3);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "samplerBRDFLUT");
		if(uniform != -1) glUniform1i(uniform, 4);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "colorMap");
		if(uniform != -1) glUniform1i(uniform, 5);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "physicalDescriptorMap");
		if(uniform != -1) glUniform1i(uniform, 6);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "normalMap");
		if(uniform != -1) glUniform1i(uniform, 7);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "aoMap");
		if(uniform != -1) glUniform1i(uniform, 8);
	}
	{
		unsigned int uniform = glGetUniformLocation(out->pbr, "emissiveMap");
		if(uniform != -1) glUniform1i(uniform, 9);
	}
	out->_internal_uniforms.idx_pbr_material = glGetUniformBlockIndex(out->pbr, "material");
	out->pbr_geom = _CreateProgram(PBR_VTX, PBR_GEOM_FRG);
	glUseProgram(out->pbr_geom);
	out->_internal_uniforms.idx_pbr_geom_ubo = glGetUniformBlockIndex(out->pbr_geom, "ubo");
	out->_internal_uniforms.idx_pbr_geom_model = glGetUniformLocation(out->pbr_geom, "model");
	out->_internal_uniforms.idx_pbr_geom_clipPlane = glGetUniformLocation(out->pbr_geom, "clipPlane");
	out->_internal_uniforms.idx_pbr_geom_node = glGetUniformBlockIndex(out->pbr_geom, "node");
	out->_internal_uniforms.idx_pbr_geom_material = glGetUniformBlockIndex(out->pbr_geom, "material");
	{
		unsigned int uniform = glGetUniformLocation(out->pbr_geom, "colorMap");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->s3d = _CreateProgram(S3D_VTX, S3D_FRG);
	glUseProgram(out->s3d);
	out->_internal_uniforms.idx_s3d_projection = glGetUniformLocation(out->s3d, "projection");
	out->_internal_uniforms.idx_s3d_model = glGetUniformLocation(out->s3d, "model");
	out->_internal_uniforms.idx_s3d_view = glGetUniformLocation(out->s3d, "view");
	out->_internal_uniforms.idx_s3d_clipPlane = glGetUniformLocation(out->s3d, "clipPlane");
	{
		unsigned int uniform = glGetUniformLocation(out->s3d, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
	out->s3d_geom = _CreateProgram(S3D_VTX, EMPTY_FRG);
	glUseProgram(out->s3d_geom);
	out->_internal_uniforms.idx_s3d_geom_projection = glGetUniformLocation(out->s3d_geom, "projection");
	out->_internal_uniforms.idx_s3d_geom_model = glGetUniformLocation(out->s3d_geom, "model");
	out->_internal_uniforms.idx_s3d_geom_view = glGetUniformLocation(out->s3d_geom, "view");
	out->_internal_uniforms.idx_s3d_geom_clipPlane = glGetUniformLocation(out->s3d_geom, "clipPlane");
	out->ui = _CreateProgram(UI_VTX, UI_FRG);
	glUseProgram(out->ui);
	{
		unsigned int uniform = glGetUniformLocation(out->ui, "tex");
		if(uniform != -1) glUniform1i(uniform, 0);
	}
}
void SH_CleanUp(struct Shader* sh) {
	glDeleteProgram(sh->reflect);
	glDeleteProgram(sh->reflect_geom);
	glDeleteProgram(sh->ao);
	glDeleteProgram(sh->blur);
	glDeleteProgram(sh->bloom);
	glDeleteProgram(sh->copy);
	glDeleteProgram(sh->dual_copy);
	glDeleteProgram(sh->upsample);
	glDeleteProgram(sh->cubemap);
	glDeleteProgram(sh->pbr);
	glDeleteProgram(sh->pbr_geom);
	glDeleteProgram(sh->s3d);
	glDeleteProgram(sh->s3d_geom);
	glDeleteProgram(sh->ui);
}
void SH_Reset(struct Shader* sh) {
	memset(sh->_internal_uniforms.reflect_has_val, 0, sizeof(sh->_internal_uniforms.reflect_has_val));
	memset(sh->_internal_uniforms.reflect_geom_has_val, 0, sizeof(sh->_internal_uniforms.reflect_geom_has_val));
	memset(sh->_internal_uniforms.ao_has_val, 0, sizeof(sh->_internal_uniforms.ao_has_val));
	memset(sh->_internal_uniforms.blur_has_val, 0, sizeof(sh->_internal_uniforms.blur_has_val));
	memset(sh->_internal_uniforms.bloom_has_val, 0, sizeof(sh->_internal_uniforms.bloom_has_val));
	memset(sh->_internal_uniforms.copy_has_val, 0, sizeof(sh->_internal_uniforms.copy_has_val));
	memset(sh->_internal_uniforms.dual_copy_has_val, 0, sizeof(sh->_internal_uniforms.dual_copy_has_val));
	memset(sh->_internal_uniforms.upsample_has_val, 0, sizeof(sh->_internal_uniforms.upsample_has_val));
	memset(sh->_internal_uniforms.cubemap_has_val, 0, sizeof(sh->_internal_uniforms.cubemap_has_val));
	memset(sh->_internal_uniforms.pbr_has_val, 0, sizeof(sh->_internal_uniforms.pbr_has_val));
	memset(sh->_internal_uniforms.pbr_geom_has_val, 0, sizeof(sh->_internal_uniforms.pbr_geom_has_val));
	memset(sh->_internal_uniforms.s3d_has_val, 0, sizeof(sh->_internal_uniforms.s3d_has_val));
	memset(sh->_internal_uniforms.s3d_geom_has_val, 0, sizeof(sh->_internal_uniforms.s3d_geom_has_val));
	memset(sh->_internal_uniforms.ui_has_val, 0, sizeof(sh->_internal_uniforms.ui_has_val));
}
void SHB_reflect_projection(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.reflect_projection, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_projection, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_projection, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 0);
}
void SHB_reflect_view(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.reflect_view, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_view, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_view, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 1);
}
void SHB_reflect_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.reflect_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 2);
}
void SHB_reflect_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 3))){
		if(memcmp(sh->_internal_uniforms.reflect_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_reflect_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.reflect_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 3);
}
void SHB_reflect__lightData(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 4))){
		if(sh->_internal_uniforms.reflect__lightData.buffer == buf && sh->_internal_uniforms.reflect__lightData.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_reflect__lightData, buf, offset, sz);
	sh->_internal_uniforms.reflect__lightData.buffer = buf;
	sh->_internal_uniforms.reflect__lightData.offset = offset;
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 4);
}
void SHB_reflect_shadowMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 5))){
		if(sh->_internal_uniforms.reflect_shadowMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.reflect_shadowMap = tex;
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 5);
}
void SHB_reflect__my_internalAOMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 6))){
		if(sh->_internal_uniforms.reflect__my_internalAOMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.reflect__my_internalAOMap = tex;
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 6);
}
void SHB_reflect_currentFBOSize(struct Shader* sh, float vals[2]) {
	if((sh->_internal_uniforms.reflect_has_val[0] & (1u << 7))){
		if(memcmp(sh->_internal_uniforms.reflect_currentFBOSize, vals, sizeof(float) * 2) == 0) return;
	}
	glUniform2fv(sh->_internal_uniforms.idx_reflect_currentFBOSize, 1, vals);
	memcpy(sh->_internal_uniforms.reflect_currentFBOSize, vals, sizeof(float) * 2);
	sh->_internal_uniforms.reflect_has_val[0] |= (1u << 7);
}
void SHB_reflect_material(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.reflect_has_val[1] & (1u << 0))){
		if(sh->_internal_uniforms.reflect_material.buffer == buf && sh->_internal_uniforms.reflect_material.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_reflect_material, buf, offset, sz);
	sh->_internal_uniforms.reflect_material.buffer = buf;
	sh->_internal_uniforms.reflect_material.offset = offset;
	sh->_internal_uniforms.reflect_has_val[1] |= (1u << 0);
}
void SHB_reflect_reflectTexture(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.reflect_has_val[1] & (1u << 1))){
		if(sh->_internal_uniforms.reflect_reflectTexture == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.reflect_reflectTexture = tex;
	sh->_internal_uniforms.reflect_has_val[1] |= (1u << 1);
}
void SHB_reflect_refractTexture(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.reflect_has_val[1] & (1u << 2))){
		if(sh->_internal_uniforms.reflect_refractTexture == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.reflect_refractTexture = tex;
	sh->_internal_uniforms.reflect_has_val[1] |= (1u << 2);
}
void SHB_reflect_dvdu(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.reflect_has_val[1] & (1u << 3))){
		if(sh->_internal_uniforms.reflect_dvdu == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.reflect_dvdu = tex;
	sh->_internal_uniforms.reflect_has_val[1] |= (1u << 3);
}
void SHB_reflect_geom_projection(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_geom_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.reflect_geom_projection, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_geom_projection, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_geom_projection, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_geom_has_val[0] |= (1u << 0);
}
void SHB_reflect_geom_view(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_geom_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.reflect_geom_view, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_geom_view, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_geom_view, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_geom_has_val[0] |= (1u << 1);
}
void SHB_reflect_geom_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.reflect_geom_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.reflect_geom_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_reflect_geom_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.reflect_geom_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.reflect_geom_has_val[0] |= (1u << 2);
}
void SHB_reflect_geom_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.reflect_geom_has_val[0] & (1u << 3))){
		if(memcmp(sh->_internal_uniforms.reflect_geom_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_reflect_geom_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.reflect_geom_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.reflect_geom_has_val[0] |= (1u << 3);
}
void SHB_ao_screenResolution(struct Shader* sh, float vals[2]) {
	if((sh->_internal_uniforms.ao_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.ao_screenResolution, vals, sizeof(float) * 2) == 0) return;
	}
	glUniform2fv(sh->_internal_uniforms.idx_ao_screenResolution, 1, vals);
	memcpy(sh->_internal_uniforms.ao_screenResolution, vals, sizeof(float) * 2);
	sh->_internal_uniforms.ao_has_val[0] |= (1u << 0);
}
void SHB_ao_gDepthMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.ao_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.ao_gDepthMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.ao_gDepthMap = tex;
	sh->_internal_uniforms.ao_has_val[0] |= (1u << 1);
}
void SHB_ao_gRandTex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.ao_has_val[0] & (1u << 2))){
		if(sh->_internal_uniforms.ao_gRandTex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.ao_gRandTex = tex;
	sh->_internal_uniforms.ao_has_val[0] |= (1u << 2);
}
void SHB_blur_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.blur_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.blur_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.blur_tex = tex;
	sh->_internal_uniforms.blur_has_val[0] |= (1u << 0);
}
void SHB_blur_blurRadius(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.blur_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.blur_blurRadius == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_blur_blurRadius, val);
	sh->_internal_uniforms.blur_blurRadius = val;
	sh->_internal_uniforms.blur_has_val[0] |= (1u << 1);
}
void SHB_blur_axis(struct Shader* sh, int val) {
	if((sh->_internal_uniforms.blur_has_val[0] & (1u << 2))){
		if(sh->_internal_uniforms.blur_axis == val) return;
	}
	glUniform1i(sh->_internal_uniforms.idx_blur_axis, val);
	sh->_internal_uniforms.blur_axis = val;
	sh->_internal_uniforms.blur_has_val[0] |= (1u << 2);
}
void SHB_blur_texUV(struct Shader* sh, float vals[2]) {
	if((sh->_internal_uniforms.blur_has_val[0] & (1u << 3))){
		if(memcmp(sh->_internal_uniforms.blur_texUV, vals, sizeof(float) * 2) == 0) return;
	}
	glUniform2fv(sh->_internal_uniforms.idx_blur_texUV, 1, vals);
	memcpy(sh->_internal_uniforms.blur_texUV, vals, sizeof(float) * 2);
	sh->_internal_uniforms.blur_has_val[0] |= (1u << 3);
}
void SHB_bloom_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.bloom_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.bloom_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.bloom_tex = tex;
	sh->_internal_uniforms.bloom_has_val[0] |= (1u << 0);
}
void SHB_bloom_blurRadius(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.bloom_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.bloom_blurRadius == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_bloom_blurRadius, val);
	sh->_internal_uniforms.bloom_blurRadius = val;
	sh->_internal_uniforms.bloom_has_val[0] |= (1u << 1);
}
void SHB_bloom_axis(struct Shader* sh, int val) {
	if((sh->_internal_uniforms.bloom_has_val[0] & (1u << 2))){
		if(sh->_internal_uniforms.bloom_axis == val) return;
	}
	glUniform1i(sh->_internal_uniforms.idx_bloom_axis, val);
	sh->_internal_uniforms.bloom_axis = val;
	sh->_internal_uniforms.bloom_has_val[0] |= (1u << 2);
}
void SHB_bloom_intensity(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.bloom_has_val[0] & (1u << 3))){
		if(sh->_internal_uniforms.bloom_intensity == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_bloom_intensity, val);
	sh->_internal_uniforms.bloom_intensity = val;
	sh->_internal_uniforms.bloom_has_val[0] |= (1u << 3);
}
void SHB_bloom_mipLevel(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.bloom_has_val[0] & (1u << 4))){
		if(sh->_internal_uniforms.bloom_mipLevel == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_bloom_mipLevel, val);
	sh->_internal_uniforms.bloom_mipLevel = val;
	sh->_internal_uniforms.bloom_has_val[0] |= (1u << 4);
}
void SHB_copy_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.copy_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.copy_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.copy_tex = tex;
	sh->_internal_uniforms.copy_has_val[0] |= (1u << 0);
}
void SHB_copy_mipLevel(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.copy_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.copy_mipLevel == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_copy_mipLevel, val);
	sh->_internal_uniforms.copy_mipLevel = val;
	sh->_internal_uniforms.copy_has_val[0] |= (1u << 1);
}
void SHB_dual_copy_tex1(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.dual_copy_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.dual_copy_tex1 == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.dual_copy_tex1 = tex;
	sh->_internal_uniforms.dual_copy_has_val[0] |= (1u << 0);
}
void SHB_dual_copy_tex2(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.dual_copy_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.dual_copy_tex2 == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.dual_copy_tex2 = tex;
	sh->_internal_uniforms.dual_copy_has_val[0] |= (1u << 1);
}
void SHB_dual_copy_mipLevel1(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.dual_copy_has_val[0] & (1u << 2))){
		if(sh->_internal_uniforms.dual_copy_mipLevel1 == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_dual_copy_mipLevel1, val);
	sh->_internal_uniforms.dual_copy_mipLevel1 = val;
	sh->_internal_uniforms.dual_copy_has_val[0] |= (1u << 2);
}
void SHB_dual_copy_mipLevel2(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.dual_copy_has_val[0] & (1u << 3))){
		if(sh->_internal_uniforms.dual_copy_mipLevel2 == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_dual_copy_mipLevel2, val);
	sh->_internal_uniforms.dual_copy_mipLevel2 = val;
	sh->_internal_uniforms.dual_copy_has_val[0] |= (1u << 3);
}
void SHB_upsample_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.upsample_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.upsample_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.upsample_tex = tex;
	sh->_internal_uniforms.upsample_has_val[0] |= (1u << 0);
}
void SHB_upsample_mipLevel(struct Shader* sh, float val) {
	if((sh->_internal_uniforms.upsample_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.upsample_mipLevel == val) return;
	}
	glUniform1f(sh->_internal_uniforms.idx_upsample_mipLevel, val);
	sh->_internal_uniforms.upsample_mipLevel = val;
	sh->_internal_uniforms.upsample_has_val[0] |= (1u << 1);
}
void SHB_cubemap_viewProj(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.cubemap_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.cubemap_viewProj, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_cubemap_viewProj, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.cubemap_viewProj, vals, sizeof(float) * 16);
	sh->_internal_uniforms.cubemap_has_val[0] |= (1u << 0);
}
void SHB_cubemap_skybox(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.cubemap_has_val[0] & (1u << 1))){
		if(sh->_internal_uniforms.cubemap_skybox == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	sh->_internal_uniforms.cubemap_skybox = tex;
	sh->_internal_uniforms.cubemap_has_val[0] |= (1u << 1);
}
void SHB_pbr_ubo(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.pbr_ubo.buffer == buf && sh->_internal_uniforms.pbr_ubo.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_ubo, buf, offset, sz);
	sh->_internal_uniforms.pbr_ubo.buffer = buf;
	sh->_internal_uniforms.pbr_ubo.offset = offset;
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 0);
}
void SHB_pbr_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.pbr_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_pbr_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.pbr_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 1);
}
void SHB_pbr_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.pbr_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_pbr_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.pbr_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 2);
}
void SHB_pbr_node(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 3))){
		if(sh->_internal_uniforms.pbr_node.buffer == buf && sh->_internal_uniforms.pbr_node.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_node, buf, offset, sz);
	sh->_internal_uniforms.pbr_node.buffer = buf;
	sh->_internal_uniforms.pbr_node.offset = offset;
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 3);
}
void SHB_pbr__lightData(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 4))){
		if(sh->_internal_uniforms.pbr__lightData.buffer == buf && sh->_internal_uniforms.pbr__lightData.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr__lightData, buf, offset, sz);
	sh->_internal_uniforms.pbr__lightData.buffer = buf;
	sh->_internal_uniforms.pbr__lightData.offset = offset;
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 4);
}
void SHB_pbr_shadowMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 5))){
		if(sh->_internal_uniforms.pbr_shadowMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_shadowMap = tex;
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 5);
}
void SHB_pbr__my_internalAOMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 6))){
		if(sh->_internal_uniforms.pbr__my_internalAOMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 1);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr__my_internalAOMap = tex;
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 6);
}
void SHB_pbr_currentFBOSize(struct Shader* sh, float vals[2]) {
	if((sh->_internal_uniforms.pbr_has_val[0] & (1u << 7))){
		if(memcmp(sh->_internal_uniforms.pbr_currentFBOSize, vals, sizeof(float) * 2) == 0) return;
	}
	glUniform2fv(sh->_internal_uniforms.idx_pbr_currentFBOSize, 1, vals);
	memcpy(sh->_internal_uniforms.pbr_currentFBOSize, vals, sizeof(float) * 2);
	sh->_internal_uniforms.pbr_has_val[0] |= (1u << 7);
}
void SHB_pbr_uboParams(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 0))){
		if(sh->_internal_uniforms.pbr_uboParams.buffer == buf && sh->_internal_uniforms.pbr_uboParams.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_uboParams, buf, offset, sz);
	sh->_internal_uniforms.pbr_uboParams.buffer = buf;
	sh->_internal_uniforms.pbr_uboParams.offset = offset;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 0);
}
void SHB_pbr_samplerIrradiance(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 1))){
		if(sh->_internal_uniforms.pbr_samplerIrradiance == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 2);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	sh->_internal_uniforms.pbr_samplerIrradiance = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 1);
}
void SHB_pbr_prefilteredMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 2))){
		if(sh->_internal_uniforms.pbr_prefilteredMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 3);
	glBindTexture(GL_TEXTURE_CUBE_MAP, tex);
	sh->_internal_uniforms.pbr_prefilteredMap = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 2);
}
void SHB_pbr_samplerBRDFLUT(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 3))){
		if(sh->_internal_uniforms.pbr_samplerBRDFLUT == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 4);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_samplerBRDFLUT = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 3);
}
void SHB_pbr_colorMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 4))){
		if(sh->_internal_uniforms.pbr_colorMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 5);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_colorMap = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 4);
}
void SHB_pbr_physicalDescriptorMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 5))){
		if(sh->_internal_uniforms.pbr_physicalDescriptorMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 6);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_physicalDescriptorMap = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 5);
}
void SHB_pbr_normalMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 6))){
		if(sh->_internal_uniforms.pbr_normalMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 7);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_normalMap = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 6);
}
void SHB_pbr_aoMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[1] & (1u << 7))){
		if(sh->_internal_uniforms.pbr_aoMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 8);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_aoMap = tex;
	sh->_internal_uniforms.pbr_has_val[1] |= (1u << 7);
}
void SHB_pbr_emissiveMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_has_val[2] & (1u << 0))){
		if(sh->_internal_uniforms.pbr_emissiveMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 9);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_emissiveMap = tex;
	sh->_internal_uniforms.pbr_has_val[2] |= (1u << 0);
}
void SHB_pbr_material(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_has_val[2] & (1u << 1))){
		if(sh->_internal_uniforms.pbr_material.buffer == buf && sh->_internal_uniforms.pbr_material.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_material, buf, offset, sz);
	sh->_internal_uniforms.pbr_material.buffer = buf;
	sh->_internal_uniforms.pbr_material.offset = offset;
	sh->_internal_uniforms.pbr_has_val[2] |= (1u << 1);
}
void SHB_pbr_geom_ubo(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.pbr_geom_ubo.buffer == buf && sh->_internal_uniforms.pbr_geom_ubo.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_geom_ubo, buf, offset, sz);
	sh->_internal_uniforms.pbr_geom_ubo.buffer = buf;
	sh->_internal_uniforms.pbr_geom_ubo.offset = offset;
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 0);
}
void SHB_pbr_geom_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.pbr_geom_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_pbr_geom_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.pbr_geom_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 1);
}
void SHB_pbr_geom_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.pbr_geom_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_pbr_geom_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.pbr_geom_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 2);
}
void SHB_pbr_geom_node(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 3))){
		if(sh->_internal_uniforms.pbr_geom_node.buffer == buf && sh->_internal_uniforms.pbr_geom_node.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_geom_node, buf, offset, sz);
	sh->_internal_uniforms.pbr_geom_node.buffer = buf;
	sh->_internal_uniforms.pbr_geom_node.offset = offset;
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 3);
}
void SHB_pbr_geom_material(struct Shader* sh, unsigned int buf, unsigned int offset, size_t sz) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 4))){
		if(sh->_internal_uniforms.pbr_geom_material.buffer == buf && sh->_internal_uniforms.pbr_geom_material.offset == offset) return;
	}
	glBindBufferRange(GL_UNIFORM_BUFFER, sh->_internal_uniforms.idx_pbr_geom_material, buf, offset, sz);
	sh->_internal_uniforms.pbr_geom_material.buffer = buf;
	sh->_internal_uniforms.pbr_geom_material.offset = offset;
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 4);
}
void SHB_pbr_geom_colorMap(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.pbr_geom_has_val[0] & (1u << 5))){
		if(sh->_internal_uniforms.pbr_geom_colorMap == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.pbr_geom_colorMap = tex;
	sh->_internal_uniforms.pbr_geom_has_val[0] |= (1u << 5);
}
void SHB_s3d_projection(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.s3d_projection, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_projection, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_projection, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_has_val[0] |= (1u << 0);
}
void SHB_s3d_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.s3d_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_has_val[0] |= (1u << 1);
}
void SHB_s3d_view(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.s3d_view, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_view, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_view, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_has_val[0] |= (1u << 2);
}
void SHB_s3d_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.s3d_has_val[0] & (1u << 3))){
		if(memcmp(sh->_internal_uniforms.s3d_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_s3d_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.s3d_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.s3d_has_val[0] |= (1u << 3);
}
void SHB_s3d_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.s3d_has_val[0] & (1u << 4))){
		if(sh->_internal_uniforms.s3d_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.s3d_tex = tex;
	sh->_internal_uniforms.s3d_has_val[0] |= (1u << 4);
}
void SHB_s3d_geom_projection(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_geom_has_val[0] & (1u << 0))){
		if(memcmp(sh->_internal_uniforms.s3d_geom_projection, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_geom_projection, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_geom_projection, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_geom_has_val[0] |= (1u << 0);
}
void SHB_s3d_geom_model(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_geom_has_val[0] & (1u << 1))){
		if(memcmp(sh->_internal_uniforms.s3d_geom_model, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_geom_model, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_geom_model, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_geom_has_val[0] |= (1u << 1);
}
void SHB_s3d_geom_view(struct Shader* sh, float vals[16]) {
	if((sh->_internal_uniforms.s3d_geom_has_val[0] & (1u << 2))){
		if(memcmp(sh->_internal_uniforms.s3d_geom_view, vals, sizeof(float) * 16) == 0) return;
	}
	glUniformMatrix4fv(sh->_internal_uniforms.idx_s3d_geom_view, 1, GL_FALSE, vals);
	memcpy(sh->_internal_uniforms.s3d_geom_view, vals, sizeof(float) * 16);
	sh->_internal_uniforms.s3d_geom_has_val[0] |= (1u << 2);
}
void SHB_s3d_geom_clipPlane(struct Shader* sh, float vals[4]) {
	if((sh->_internal_uniforms.s3d_geom_has_val[0] & (1u << 3))){
		if(memcmp(sh->_internal_uniforms.s3d_geom_clipPlane, vals, sizeof(float) * 4) == 0) return;
	}
	glUniform4fv(sh->_internal_uniforms.idx_s3d_geom_clipPlane, 1, vals);
	memcpy(sh->_internal_uniforms.s3d_geom_clipPlane, vals, sizeof(float) * 4);
	sh->_internal_uniforms.s3d_geom_has_val[0] |= (1u << 3);
}
void SHB_ui_tex(struct Shader* sh, unsigned int tex) {
	if((sh->_internal_uniforms.ui_has_val[0] & (1u << 0))){
		if(sh->_internal_uniforms.ui_tex == tex) return;
	}
	glActiveTexture(GL_TEXTURE0 + 0);
	glBindTexture(GL_TEXTURE_2D, tex);
	sh->_internal_uniforms.ui_tex = tex;
	sh->_internal_uniforms.ui_has_val[0] |= (1u << 0);
}

