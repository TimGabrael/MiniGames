#include "PluginLoader.h"
#include <filesystem>
#include "logging.h"

#ifdef _WIN32
#define WIN32_LEAN_AND_MEAN
#include <Windows.h>
#define DYNAMIC_LIBRARY_EXTENSION ".dll"
#define LOAD(file) (void*)LoadLibraryA(file)
#define SYM(handle, procName) (void*)GetProcAddress((HMODULE)handle, procName)
#define FREE(handle) FreeLibrary((HMODULE)handle);
#else
#include <dlfcn.h>
#define DYNAMIC_LIBRARY_EXTENSION ".so"
#define LOAD(file) dlopen(file, RTLD_NOW)
#define SYM(handle, procName) dlsym((void*)handle, procName)
#define FREE(handle) dlclose((void*)handle)
#endif

#undef min
#undef max

typedef PluginClass*(*PluginCreateFunction)();
static std::vector<PluginClass*> plug;

void LoadAllPlugins()
{
	for (auto& p : plug)
	{
		delete p;
	}
	plug.clear();
	std::filesystem::path pluginPath("Plugins");
	std::filesystem::directory_iterator iter(pluginPath);
	for (auto& f : iter)
	{
		if (f.is_regular_file() && f.path().has_extension() && f.path().extension() == DYNAMIC_LIBRARY_EXTENSION) {
			std::string file = f.path().string();
			void* outModule = LOAD(file.c_str());
			if (outModule)
			{
				PluginCreateFunction plugFunc = (PluginCreateFunction)SYM(outModule, "GetPlugin");
				if (plugFunc)
				{
					plug.push_back(plugFunc());
				}
				else
				{
					FREE(outModule);
				}
			}
		}
	}
}

const std::vector<PluginClass*>& GetPlugins()
{
	return plug;
}

void LoadAndFilterPlugins(const std::vector<std::string>& used)
{
	LoadAllPlugins();
	std::vector<PluginClass*> remainingPlugins;
	for (auto p : plug)
	{
		PLUGIN_INFO pl = p->GetPluginInfos();
		bool wasFound = false;
		for (int i = 0; i < used.size(); i++)
		{
			if (memcmp(pl.ID, used.at(i).data(), std::min((int)used.at(i).size(), 19))) {
				wasFound = true;
				break;
			}
		}
		if (wasFound)
		{
			remainingPlugins.push_back(p);
		}
		else
		{
			delete p;
		}
	}
	plug = std::move(remainingPlugins);
}