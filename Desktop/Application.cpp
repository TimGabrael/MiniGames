#include "Application.h"
#include <qfile.h>
#include <qregularexpression.h>




MainApplication* g_mainApplicationInstance = nullptr;
MainApplication::MainApplication(int& argc, char** argv) : QApplication(argc, argv)
{
	g_mainApplicationInstance = this;

}
MainApplication::~MainApplication()
{

}
MainApplication* MainApplication::GetInstance()
{
	return g_mainApplicationInstance;
}