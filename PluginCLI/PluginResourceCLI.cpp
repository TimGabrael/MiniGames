#include <vector>
#include <string>
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>


void PrintHelpScreen()
{
	std::cout << "--help -h: print help\n";
	std::cout << "--outdir -o: set output directory\n";
	std::cout << "--logo -l: required Logo Image\n";
	std::cout << "--resourcePath -res: path to the resources that get embedded\n";
	std::cout << "--pluginID -id: required PluginID needs to be exactly 16 Characters long\n";
	std::cout << "--allowAsyncLoad -load: flag allows to join late\n";
	std::cout << "--maxPlayerCount -max: maximum player count\n";
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
		if (strnicmp(argv[i], "-h", 3) == 0 || strnicmp(argv[i], "--help", 7) == 0)
		{
			// print help screen
			PrintHelpScreen();
			return 0;
		}
		else if (!serverInfo.canJoinAfterLoad && (strnicmp(argv[i], "-load", 6) == 0 || strnicmp(argv[i], "--allowAsyncLoad", 17) == 0))
		{
			serverInfo.canJoinAfterLoad = true;
		}
		if ((i + 1) < argc)
		{
			if (!outdir && (strnicmp(argv[i], "-o", 3) == 0 || strnicmp(argv[i], "--outdir", 9) == 0))
			{
				outdir = argv[i + 1];
			}
			else if (!logoPath && (strnicmp(argv[i], "-l", 3) == 0 || strnicmp(argv[i], "--logo", 7) == 0))
			{
				logoPath = argv[i + 1];
			}
			else if (!resourcePath && (strnicmp(argv[i], "-res", 5) == 0 || strnicmp(argv[i], "--resourcePath", 15) == 0))
			{
				resourcePath = argv[i + 1];
			}
			else if (!pluginID && (strnicmp(argv[i], "-id", 4) == 0 || strnicmp(argv[i], "--pluginID", 11) == 0))
			{
				pluginID = argv[i + 1];
			}
			else if (strnicmp(argv[i], "-max", 5) == 0 || strnicmp(argv[i], "--maxPlayerCount", 17) == 0)
			{
				serverInfo.maxPlayerCount = atoi(argv[i + 1]);
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

	int curDataHead = headerSize;

	// Write Server Info
	{
		outfile.write((const char*)&serverInfo, sizeof(ServerResourceFileInfo));
	}
	// Write Logo Info
	{
		std::ifstream file(logoPath, std::ifstream::binary | std::ios::ate);
		if (!file.is_open()) { std::cout << "ERROR: Opening Logo " << logoPath << std::endl; return 0; }
		int filesize = file.tellg();
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
		int filesize = file.tellg();
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