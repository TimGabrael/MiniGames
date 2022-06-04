#pragma once
#include <qwidget.h>
#include <qmainwindow.h>

class LobbyFrame : public QWidget
{
public:
	LobbyFrame(QMainWindow* parent);
	~LobbyFrame();

private:
	QWidget* playerScrollContent = nullptr;
};