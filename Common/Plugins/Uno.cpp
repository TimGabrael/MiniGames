#include "Uno.h"
#include <string>
#include "Graphics/Helper.h"

PLUGIN_EXPORT_DEFINITION(UnoPlugin, "a3fV-6giK-10Eb-2rdT");

PLUGIN_INFO UnoPlugin::GetPluginInfos()
{
	PLUGIN_INFO plugInfo;
	memcpy(plugInfo.ID, _Plugin_Export_ID_Value, strnlen_s(_Plugin_Export_ID_Value, 19));
	plugInfo.previewResource = nullptr;
	return plugInfo;
}
void UnoPlugin::PluginInit(void* backendData)
{
	InitializeOpenGL();
}
void UnoPlugin::PluginRender(void* backendData) 
{

}