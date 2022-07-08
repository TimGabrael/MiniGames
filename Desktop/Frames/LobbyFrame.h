#pragma once
#include <qwidget.h>
#include <qmainwindow.h>

class LobbyFrame : public QWidget
{
public:
	LobbyFrame(QMainWindow* parent);
	~LobbyFrame();

	void StartPlugin(int idx);
private:
	QWidget* playerScrollContent = nullptr;
	
};