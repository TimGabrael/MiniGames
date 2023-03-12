#include <corecrt_math.h>
#include <future>
#include <stdio.h>
#include <string.h>
#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <thread>
#include <algorithm>
#define _USE_MATH_DEFINES
#include <math.h>
#define STB_IMAGE_WRITE_IMPLEMENTATION
#include "stb_image_write.h"
#define STB_IMAGE_IMPLEMENTATION
#include "../Common/Graphics/stb_image.h"


void PrintHelpScreen()
{
	std::cout << "--help -h: print help\n";
	std::cout << "--outdir -o: set output directory\n";
	std::cout << "--logo -l: required Logo Image\n";
	std::cout << "--resourcePath -res: path to the resources that get embedded\n";
	std::cout << "--pluginID -id: required PluginID needs to be exactly 16 Characters long\n";
	std::cout << "--allowAsyncLoad -load: flag allows to join late\n";
	std::cout << "--maxPlayerCount -max: maximum player count\n";
	std::cout << "--environment -env: generate environment maps from hdr images\nfile order: bottom, back, front, left, right, top\n";
}

void ParseAllResources(const std::filesystem::path& path, std::vector<std::string>& fill)
{
	std::filesystem::directory_iterator iter(path);
	for (auto& f : iter)
	{
		if (f.is_directory())
		{
			ParseAllResources(f.path(), fill);
		}
		else if (f.is_regular_file())
		{
			fill.push_back(f.path().string());
		}

	}
}
struct ServerResourceFileInfo
{
	uint32_t maxPlayerCount;
	bool canJoinAfterLoad;
};
struct ResourceFileImageInfo
{
	int index;
	int size;
};

struct fvec3 {
    float x, y, z;
};

struct CubeSampleInfo
{
    fvec3 start;
    fvec3 dx;
    fvec3 dy;
};

static constexpr CubeSampleInfo sampleInfo[] = {
    {{ 1.0f, 1.0f, 1.0f},{ 0.0f, 0.0f,-2.0f},{ 0.0f,-2.0f, 0.0f}}, // bottom
    {{-1.0f, 1.0f,-1.0f},{ 0.0f, 0.0f, 2.0f},{ 0.0f,-2.0f, 0.0f}}, // back
    {{-1.0f, 1.0f,-1.0f},{ 2.0f, 0.0f, 0.0f},{ 0.0f, 0.0f, 2.0f}}, // front
    {{-1.0f,-1.0f, 1.0f},{ 2.0f, 0.0f, 0.0f},{ 0.0f, 0.0f,-2.0f}}, // left
    {{-1.0f,-1.0f, 1.0f},{ 2.0f, 0.0f, 0.0f},{ 0.0f, 2.0f, 0.0f}}, // right
    {{ 1.0f,-1.0f,-1.0f},{-2.0f, 0.0f, 0.0f},{ 0.0f, 2.0f, 0.0f}}, // top
};


inline static fvec3 SampleCubemap(float** imgs, const fvec3& sample, int w, int h) {
    const fvec3 as = { abs(sample.x), abs(sample.y), abs(sample.z)  };
    int idx = 0;
    float ma = 0.0f;
    float u = 0.0f;
    float v = 0.0f;
    if(as.z >= as.x && as.z >= as.y) {
        idx = sample.z < 0.0f ? 5 : 4;
        ma = 0.5f / as.z;
        u = sample.z < 0.0f ? -sample.x : sample.x;
        v = sample.y;
    }
    else if(as.y >= as.x) {
        idx = sample.y < 0.0f ? 3 : 2;
        ma = 0.5f / as.y;
        u = sample.x;
        v = sample.y < 0.0f ? -sample.z : sample.z;
    }
    else {
        idx = sample.x < 0.0f ? 1 : 0;
        ma = 0.5f / as.x;
        u = sample.x < 0.0f ? sample.z : -sample.z;
        v = -sample.y;
    }
    u = u * ma + 0.5f;
    v = v * ma + 0.5f;
    const int x = std::min(std::max((int)(w * u), 0), w - 1);
    const int y = std::min(std::max((int)(h * v), 0), h - 1);
    const float r = imgs[idx][4 * (x + y * w) + 0];
    const float g = imgs[idx][4 * (x + y * w) + 1];
    const float b = imgs[idx][4 * (x + y * w) + 2];

    return {r, g, b};
}
void ENV_GenIrradiance(float* out, float** imgs, int irr_w, int irr_h, int w, int h, float dphi, float dthe, int side) {
    const CubeSampleInfo info = sampleInfo[side];
    const float dx = 1.0f / (float)irr_w;
    const float dy = 1.0f / (float)irr_h;
    const float TWO_PI = 2.0f * (float)M_PI;
    const float HALF_PI = 0.5f * (float)M_PI;
    struct Phi {
        float cp;
        float sp;
    };
    struct Theta {
        float ct;
        float st;
    };
    const int num_phis = (int)(TWO_PI / dphi);
    const int num_thes = (int)(HALF_PI / dthe);
    if(num_phis >= 400) return;
    if(num_thes >= 200) return;
    Phi phis[400];
    Theta thes[200];
    for(int i = 0; i < num_phis; i++) {
        phis[i].cp = cosf(dphi * i);
        phis[i].sp = sinf(dphi * i);
    }
    for(int i = 0; i < num_thes; i++) {
        thes[i].ct = cosf(dthe * i);
        thes[i].st = sinf(dthe * i);
    }
    const float scale_factor = (float)M_PI / (float)(num_phis * num_thes);

    for(int y = 0; y < irr_h; y++){
        const float fy = y * dy;
        for(int x = 0; x < irr_w; x++) {
            const float fx = x * dx;
            const fvec3 pos = { info.start.x +  info.dx.x * fx + info.dy.x * fy, info.start.y +  info.dx.y * fx + info.dy.y * fy, info.start.z +  info.dx.z * fx + info.dy.z * fy };
            const float poslen = 1.0f / sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
            const fvec3 npos = poslen == 0.0f ? fvec3({1.0f, 0.0f, 0.0f}) : fvec3({pos.x * poslen, pos.y * poslen, pos.z * poslen});
            const fvec3 right = {npos.z, 0.0f, -npos.x};
            const float rightlen = 1.0f / sqrtf(right.x * right.x + right.y * right.y + right.z * right.z);
            const fvec3 nright = { right.x * rightlen, right.y * rightlen, right.z * rightlen };
            const fvec3 up = { npos.y * nright.z - npos.z * nright.y, npos.z * nright.x - npos.x * nright.z, npos.x * nright.y - npos.y * nright.x };

            fvec3 color = {0.0f, 0.0f, 0.0f};
            for(int p = 0; p < num_phis; p++) {
                const fvec3 temp_vec = {
                    phis[p].cp * nright.x + phis[p].sp * up.x,
                    phis[p].cp * nright.y + phis[p].sp * up.y,
                    phis[p].cp * nright.z + phis[p].sp * up.z,
                };
                for(int t = 0; t < num_thes; t++) {
                    const fvec3 sample_vec = {
                        thes[t].ct * npos.x + thes[t].st * temp_vec.x,
                        thes[t].ct * npos.y + thes[t].st * temp_vec.y,
                        thes[t].ct * npos.z + thes[t].st * temp_vec.z,
                    };
                    const fvec3 sampled = SampleCubemap(imgs, sample_vec, w, h);
                    color = { color.x + sampled.x * thes[t].ct * thes[t].st, color.y + sampled.y * thes[t].ct * thes[t].st, color.z + sampled.z * thes[t].ct * thes[t].st};
                }
            }
            const int idx = 4 * (x + y * irr_w);
            out[idx + 0] = color.x * scale_factor;
            out[idx + 1] = color.y * scale_factor;
            out[idx + 2] = color.z * scale_factor;
            out[idx + 3] = 1.0f;
        }
    }
}
float random(float x, float y) {
    const float a = 12.9898f;
	const float b = 78.233f;
	const float c = 43758.5453f;
	const float dt= x * a + y * b;
	const float sn= fmod(dt, 3.14f);
    const float o = sinf(sn) * c;
	return o - floorf(o); 
}
inline static void Hammersley2D(float* outx, float* outy, uint32_t i, uint32_t num_samples) {
    uint32_t bits = (i << 16u) | (i >> 16u);
	bits = ((bits & 0x55555555u) << 1u) | ((bits & 0xAAAAAAAAu) >> 1u);
	bits = ((bits & 0x33333333u) << 2u) | ((bits & 0xCCCCCCCCu) >> 2u);
	bits = ((bits & 0x0F0F0F0Fu) << 4u) | ((bits & 0xF0F0F0F0u) >> 4u);
	bits = ((bits & 0x00FF00FFu) << 8u) | ((bits & 0xFF00FF00u) >> 8u);
	const float rdi = float(bits) * 2.3283064365386963e-10f;
	*outx = float(i) /float(num_samples);
    *outy = rdi;
}
inline static fvec3 ImportanceSampleGGX(float xix, float xiy, float roughness, const fvec3& normal) {
    const float alpha = roughness * roughness;
    const float phi = 2.0f * (float)M_PI * xix + random(normal.x, normal.z) * 0.1f;
    const float cosTheta = sqrtf((1.0f - xiy) / (1.0f + (alpha * alpha - 1.0f) * xiy));
    const float sinTheta = sqrtf(1.0f - cosTheta * cosTheta);
    const fvec3 H = {sinTheta * cosf(phi), sinTheta * sinf(phi), cosTheta };
    const fvec3 up = abs(normal.z) < 0.999 ? fvec3({ 0.0f, 0.0f, 1.0f }) : fvec3({1.0f, 0.0f, 0.0f});
    fvec3 tangentX = { up.y * normal.z - up.z * normal.y, up.z * normal.x - up.x * normal.z, up.x * normal.y - up.y * normal.x };
    const float txlen = tangentX.x * tangentX.x + tangentX.y * tangentX.y + tangentX.z * tangentX.z;
    tangentX = { tangentX.x * txlen, tangentX.y * txlen, tangentX.z * txlen };
    fvec3 tangentY = { normal.y * tangentX.z - normal.z * tangentX.y, normal.z * tangentX.x - normal.x * tangentX.z, normal.x * tangentX.y - normal.y * tangentX.x };
    const float tylen = tangentY.x * tangentY.x + tangentY.y * tangentY.y + tangentY.z * tangentY.z;
    tangentY = { tangentY.x * tylen, tangentY.y * tylen, tangentY.z * tylen };
    fvec3 out = {
        tangentX.x * H.x + tangentY.x * H.y + normal.x * H.z,
        tangentX.y * H.x + tangentY.y * H.y + normal.y * H.z,
        tangentX.z * H.x + tangentY.z * H.y + normal.z * H.z,
    };
    const float outlen = out.x * out.x + out.y * out.y + out.z * out.z;
    out = { out.x * outlen, out.y * outlen, out.z * outlen };
    return out;
}
inline static float D_GGX(float dotNH, float roughness) {
    const float alpha = roughness * roughness;
	const float alpha2 = alpha * alpha;
	const float denom = dotNH * dotNH * (alpha2 - 1.0f) + 1.0f;
	return (alpha2)/((float)M_PI * denom*denom);
}
void ENV_GenPreFiltered(float* out, float** imgs, int pre_w, int pre_h, int w, int h, float roughness, uint32_t num_samples, int side) {
    const CubeSampleInfo info = sampleInfo[side];
    const float dx = 1.0f / (float)pre_w;
    const float dy = 1.0f / (float)pre_h;
     for(int y = 0; y < pre_h; y++){
        const float fy = y * dy;
        for(int x = 0; x < pre_w; x++) {
            const float fx = x * dx;
            const fvec3 pos = { info.start.x +  info.dx.x * fx + info.dy.x * fy, info.start.y +  info.dx.y * fx + info.dy.y * fy, info.start.z +  info.dx.z * fx + info.dy.z * fy };
            const float poslen = 1.0f / sqrtf(pos.x * pos.x + pos.y * pos.y + pos.z * pos.z);
            const fvec3 npos = {pos.x * poslen, pos.y * poslen, pos.z * poslen};
            
            fvec3 color = {0.0f, 0.0f, 0.0f};
            float totalWeight = 0.0f;
            for(uint32_t i = 0; i < num_samples; i++) {
                float xix, xiy;
                Hammersley2D(&xix, &xiy, i, num_samples);
                const fvec3 H = ImportanceSampleGGX(xix, xiy, roughness, npos);
                float dotNH = (npos.x * H.x + npos.y * H.y + npos.z * H.z);
                const fvec3 L = { 2.0f * dotNH * H.x - npos.x, 2.0f * dotNH * H.y - npos.y, 2.0f * dotNH * H.z - npos.z };
                float dotNL = npos.x * L.x + npos.y * L.y + npos.z * L.z;
                dotNL = std::max(std::min(dotNL, 1.0f), 0.0f);
                if(dotNL > 0.0f) {
                    const fvec3 tex = SampleCubemap(imgs, L, w, h);
                    color = { color.x + tex.x * dotNL, color.y + tex.y * dotNL, color.z + tex.z * dotNL };
                    totalWeight += dotNL;
                }
            }

            const int idx = 4 * (x + y * pre_w);
            totalWeight = 1.0f / totalWeight;
            out[idx + 0] = color.x * totalWeight;
            out[idx + 1] = color.y * totalWeight;
            out[idx + 2] = color.z * totalWeight;
            out[idx + 3] = 1.0f;
        }
     }

}


int main(int argc, char* argv[])
{
	ServerResourceFileInfo serverInfo;
	serverInfo.maxPlayerCount = 0xFFFFFFFF;
	serverInfo.canJoinAfterLoad = false;
	const char* outdir = nullptr;
	const char* logoPath = nullptr;
	const char* resourcePath = nullptr;
    const char* pluginID = nullptr;
    for (int i = 1; i < argc; i++)
    {
        if (_strnicmp(argv[i], "-h", 3) == 0 || _strnicmp(argv[i], "--help", 7) == 0)
        {
            // print help screen
            PrintHelpScreen();
            return 0;
        }
        else if (!serverInfo.canJoinAfterLoad && (_strnicmp(argv[i], "-load", 6) == 0 || _strnicmp(argv[i], "--allowAsyncLoad", 17) == 0))
        {
            serverInfo.canJoinAfterLoad = true;
        }
        if ((i + 1) < argc)
        {
            if (!outdir && (_strnicmp(argv[i], "-o", 3) == 0 || _strnicmp(argv[i], "--outdir", 9) == 0))
            {
                outdir = argv[i + 1];
            }
            else if (!logoPath && (_strnicmp(argv[i], "-l", 3) == 0 || _strnicmp(argv[i], "--logo", 7) == 0))
            {
                logoPath = argv[i + 1];
            }
            else if (!resourcePath && (_strnicmp(argv[i], "-res", 5) == 0 || _strnicmp(argv[i], "--resourcePath", 15) == 0))
            {
                resourcePath = argv[i + 1];
            }
            else if (!pluginID && (_strnicmp(argv[i], "-id", 4) == 0 || _strnicmp(argv[i], "--pluginID", 11) == 0))
            {
                pluginID = argv[i + 1];
            }
            else if (_strnicmp(argv[i], "-max", 5) == 0 || _strnicmp(argv[i], "--maxPlayerCount", 17) == 0)
            {
                serverInfo.maxPlayerCount = atoi(argv[i + 1]);
            }
            else if(_strnicmp(argv[i], "-env", 5) == 0 || _strnicmp(argv[i], "--environment", 14) == 0) {
                if((i + 6) < argc) {
                    // env settings
                    static constexpr float dphi = (float)M_PI * 2.0f / 180.0f;
                    static constexpr float dthe = (float)M_PI * 0.5f / 64.0f;
                    static constexpr uint32_t num_samples = 32;
                    static constexpr int irr_w = 128;
                    static constexpr int irr_h = 128;

                    int w = 0,h = 0;
                    float* buffers[6] = { 0 };
                    for(int j = 0; j < 6; j++) {
                        int cur_w, cur_h, cur_c;
                        buffers[j] = stbi_loadf(argv[i + j + 1], &cur_w, &cur_h, &cur_c, 4);
                        if(!buffers[j]) {
                            std::cout << "ERROR: Failed to open Environment Map: " << argv[i + j + 1] << "\n";
                        }
                        if(w == 0 && h == 0) {
                            w = cur_w; h = cur_h;
                        }
                        else {
                            if(w != cur_w || h != cur_h) {
                                w = -1; h = -1;
                                std::cout << "ERROR: Failed to create Environment Maps, the images are incompatible with eachother\n";
                                break;
                            }
                        }
                    }
                    if(w > 0 && h > 0) {
                        printf("Generating Environment\n");
                        int numMipMaps = (int)(1 + floor(log2(std::max(w, h))));
                        float* irradiance = new float[4 * irr_w * irr_h * 6];
                        size_t prefiltered_size = 0;
                        for(int j = 0; j < numMipMaps; j++) {
                            const int sx = std::max(w >> j, 1);
                            const int sy = std::max(h >> j, 1);
                            prefiltered_size += (size_t)(4 * sx * sy * 6);
                        }
                        float* prefiltered = new float[prefiltered_size];
                        // generate irradiance map
                        std::vector<std::future<void>> outs;
                        for(int j = 0; j < 6; j++) {
                            outs.push_back(std::async(std::launch::async, ENV_GenIrradiance, irradiance + 4 * irr_w * irr_h * j, buffers, irr_w, irr_h, w, h, dphi, dthe, j));
                        }

                        float* cur_pref = prefiltered;
                        for(int mip = 0; mip < numMipMaps; mip++) {
                            const float roughness = (float)mip / (float)(numMipMaps - 1);
                            const int cur_w = std::max(w >> mip, 1);
                            const int cur_h = std::max(h >> mip, 1);
                            for(int j = 0; j < 6; j++) {
                                outs.push_back(std::async(std::launch::async, ENV_GenPreFiltered, cur_pref, buffers, cur_w, cur_h, w, h, roughness, num_samples, j));
                                cur_pref += 4 * cur_w * cur_h;
                            }
                        }

                        outs.clear();
                        struct EnvironmentHeaderInfo {
                            int w,h;
                            int irr_w, irr_h;
                        };
                        EnvironmentHeaderInfo info;
                        info.w = w;
                        info.h = h;
                        info.irr_w = irr_w;
                        info.irr_h = irr_h;
                        std::ofstream env_file("environment.env", std::ios_base::binary);
                        if(env_file.is_open()) {
                            env_file.write((const char*)&info, sizeof(EnvironmentHeaderInfo));
                            env_file.write((const char*)prefiltered, prefiltered_size * 4);
                            env_file.write((const char*)irradiance, 4 * 6 * irr_w * irr_h * 4);
                            env_file.close();
                        }
                        

                        //stbi_write_hdr("test0.hdr", irr_w, irr_h, 4, irradiance);
                        //stbi_write_hdr("test1.hdr", irr_w, irr_h, 4, irradiance + 4 * irr_w * irr_h * 1);
                        //stbi_write_hdr("test2.hdr", irr_w, irr_h, 4, irradiance + 4 * irr_w * irr_h * 2);
                        //stbi_write_hdr("test3.hdr", irr_w, irr_h, 4, irradiance + 4 * irr_w * irr_h * 3);
                        //stbi_write_hdr("test4.hdr", irr_w, irr_h, 4, irradiance + 4 * irr_w * irr_h * 4);
                        //stbi_write_hdr("test5.hdr", irr_w, irr_h, 4, irradiance + 4 * irr_w * irr_h * 5);

                        //std::string base_str = "pretest";
                        //cur_pref = prefiltered;
                        //for(int mip = 0; mip < 1; mip++) {
                        //    int cur_w = std::max((w >> mip), 1);
                        //    int cur_h = std::max((h >> mip), 1);
                        //    for(int j = 0; j < 6; j++) {
                        //        std::string str = base_str + std::to_string(mip) + "_" + std::to_string(j) + ".hdr";
                        //        stbi_write_hdr(str.c_str(), cur_w, cur_h, 4, cur_pref);
                        //        cur_pref += 4 * cur_w * cur_h;

                        //    }
                        //}
                        printf("finished\n");
                    }

                    for(int j = 0; j < 6; j++) {
                        if(buffers[j]) {
                            stbi_image_free(buffers[j]);
                        }
                    }
                    
                }
            }
        }
    }
	const size_t pluginLen = pluginID ? strnlen(pluginID, 18) : 0;
	if (!pluginID) { std::cout << "ERROR: missing pluginID\n"; }
	if (!logoPath) { std::cout << "ERROR: missing logo\n"; }
	if (pluginID && pluginLen != 16) { std::cout << "ERROR: pluginID invalid Length\n"; }
	if (!resourcePath) { std::cout << "WARNING: no resource Path provided\n"; }
	if (!pluginID || !logoPath) { PrintHelpScreen(); return 0; }

	std::string outputPath;
	if (outdir)
	{
		outputPath = outdir + std::string("/") + pluginID + ".gres";
	}
	else
	{
		outputPath = std::string(pluginID) + ".gres";
	}
	

	std::vector<std::string> allResources;
	{
		std::filesystem::path path(resourcePath);
		ParseAllResources(path, allResources);
	}
	const size_t logoLen = strlen("*logo");
	size_t headerSize = logoLen + 2 + sizeof(ResourceFileImageInfo);	// logo internal name will be *logo as this cannot overlapp with any other file, as * is not allowed as a file name
	for (const std::string& r : allResources)
	{
		headerSize += r.size() + 2 + sizeof(ResourceFileImageInfo);	// Add the "" aswell
	}
	headerSize += sizeof(ResourceFileImageInfo);	// add  invalid file at the end, to signal the end of the header


	std::ofstream outfile(outputPath.c_str(), std::ofstream::binary);

	int curDataHead = (int)headerSize;

	// Write Server Info
	{
		outfile.write((const char*)&serverInfo, sizeof(ServerResourceFileInfo));
	}
	// Write Logo Info
	{
		std::ifstream file(logoPath, std::ifstream::binary | std::ios::ate);
		if (!file.is_open()) { std::cout << "ERROR: Opening Logo " << logoPath << std::endl; return 0; }
		int filesize = (int)file.tellg();
		file.close();
		outfile.write((const char*)&curDataHead, sizeof(int));
		outfile.write((const char*)&filesize, sizeof(int));
		outfile.write("\"*logo\"", logoLen + 2);

		curDataHead += filesize;
	}
	// Write Resources Info
	for (const std::string& r : allResources)
	{
		std::ifstream file(r.c_str(), std::ifstream::binary | std::ios::ate);
		if (!file.is_open()) { std::cout << "ERROR: Opening file " << r << std::endl; return 0; }
		int filesize = (int)file.tellg();
		file.close();

		outfile.write((const char*)&curDataHead, sizeof(int));
		outfile.write((const char*)&filesize, sizeof(int));
		outfile.write("\"", 1);
		outfile.write(r.c_str(), r.size());
		outfile.write("\"", 1);

		curDataHead += filesize;
	}
	// Write Invalid File
	{
		int invalidSizeIndex = -1;
		outfile.write((const char*)&invalidSizeIndex, sizeof(int));
		outfile.write((const char*)&invalidSizeIndex, sizeof(int));
	}
	// Write Logo Data
	{
		std::ifstream file(logoPath, std::ifstream::binary);
		if (!file.is_open()) { std::cout << "ERROR: Opening Logo " << logoPath << std::endl; return 0; }
		outfile << file.rdbuf();
		file.close();
	}
	// Write Resource Data
	for (const std::string& r : allResources)
	{
		std::ifstream file(r.c_str(), std::ifstream::binary);
		if (!file.is_open()) { std::cout << "ERROR: Opening file " << r << std::endl; return 0; }
		outfile << file.rdbuf();
		file.close();
	}

	outfile.close();


}
