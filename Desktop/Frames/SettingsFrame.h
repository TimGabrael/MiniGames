#pragma once

#include <qwidget.h>
#include <qmainwindow.h>


class SettingsFrame : public QWidget
{
public:
	SettingsFrame(QMainWindow* parent);
	~SettingsFrame();

	void Create();


private:
	
	void OnFullSceen();
	
	
	struct CustomCheckBox* fullscreen;
};