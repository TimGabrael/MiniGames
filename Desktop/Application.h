#pragma once
#include <qapplication>

class MainApplication : public QApplication
{
public:
	MainApplication(int& argc, char** argv);
	~MainApplication();


	QWidget* backgroundWidget = nullptr;

	QString username;
	QString lobbyname;

	bool fullscreen;



	static MainApplication* GetInstance();
};