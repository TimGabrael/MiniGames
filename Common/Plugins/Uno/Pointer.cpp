#include "Pointer.h"



bool Pointer::Pressed() const
{
	return pressInfo & 2;
}
bool Pointer::Released() const
{
	return (pressInfo & 0x4) && !(pressInfo & 1);
}



void Pointer::EndFrame()
{
	pressInfo = this->pressInfo << 2 | (pressInfo & 1);
}
void Pointer::FillFromMouse(const PB_MouseData* data)
{
	if (data->lPressed) pressInfo = 3;
	if (data->lUp) pressInfo &= ~3;

	this->x = data->xPos; this->y = data->yPos;
	this->dx = data->dx; this->dy = data->dy;

}
void Pointer::OnTouchDown(int x, int y, int touchID)
{
	if (!(pressInfo & 1)) 
	{
		this->id = touchID;
		this->x = x; this->y = y;
		this->dx = 0; this->dy = 0;
		pressInfo |= 3;
	}
}
void Pointer::OnTouchUp(int x, int y, int touchID)
{
	if (touchID == this->id) {
		pressInfo &= ~3;
		this->x = x;
		this->y = y;
		this->dx = 0; this->dy = 0;
		id = -1;
	}
}
void Pointer::OnTouchMove(int x, int y, int dx, int dy, int touchID)
{
	if (touchID = this->id)
	{
		this->x = x; this->y = y;
		this->dx = dx; this->dy = dy;
	}
}

