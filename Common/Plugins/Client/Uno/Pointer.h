#pragma once
#include "Plugins/Client/PluginCommon.h"
#include <stdint.h>


// only left mouse button is handled
struct Pointer
{
	int x;
	int y;
	int dx;
	int dy;


	bool Pressed() const;
	bool Released() const;




	void EndFrame();

	void FillFromMouse(const PB_MouseData* data);
	void OnTouchDown(int x, int y, int touchID);
	void OnTouchUp(int x, int y, int touchID);
	void OnTouchMove(int x, int y, int dx, int dy, int touchID);

private:
	int id;

	uint8_t pressInfo;
};