#pragma once

#include "StateFrame.h"


class SettingsFrame : public StateFrame
{
public:
	SettingsFrame(QMainWindow* parent);
	~SettingsFrame();

	void Create();



private:
	
	void OnFullSceen();
	
	
	class CustomCheckBox* fullscreen;
};
