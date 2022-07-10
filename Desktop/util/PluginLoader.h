#pragma once
#include "Plugins/PluginCommon.h"
#include <vector>

void LoadAllPlugins();

const std::vector<PluginClass*>& GetPlugins();

void LoadAndFilterPlugins(const std::vector<std::string>& used);