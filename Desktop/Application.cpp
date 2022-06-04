#include "Application.h"
#include <qfile.h>
#include <qregularexpression.h>

static const char* configTemplate =
"\
username=\"%1\"\n\
lobbyname=\"%2\"\n\
fullscreen=\"%3\"\n\
";

MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv)
{
	QRegularExpression regex("");
	g_mainApplicationInstance = this;
	QFile config("config.cfg");
	if (config.exists())
	{
		config.open(QIODeviceBase::OpenModeFlag::ReadOnly);

		QString configData = config.readAll();
		
		
	}
	else
	{
		config.open(QIODeviceBase::OpenModeFlag::ReadWrite);
		QString outData(configTemplate);
		outData = outData.arg("").arg("").arg(0);
		config.write(outData.toStdString().c_str());
	}
	if (config.isOpen())
		config.close();
}
MainApplication::~MainApplication()
{

}
MainApplication* MainApplication::GetInstance()
{
	return g_mainApplicationInstance;
}