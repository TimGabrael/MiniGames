#pragma once
#include "PluginCommon.h"
#include "../../logging.h"

struct InputButton
{
	void Press() 
	{
		state |= 3;
	}
	void Release()
	{
		state &= ~3;
	}

	void FrameEnd() {
		state = (state << 2) | (state & 1);
	}
	bool Pressed() const {
		return state & 2;
	}
	bool Released() const {
		return ((state & 4) && !(state & 1));
	}
	bool Down() const
	{
		return state & 1;
	}
private:
	//		STATE_BIT:		8				7				6				5				4				3				2				1
	//		USE:		  NONE			  NONE			  NONE			  NONE	       PREV PRESSED		PREV DOWN		 PRESSED		   DOWN
	uint8_t state;
};



struct MouseState
{
	enum BUTTONS
	{
		BTN_LEFT,
		BTN_RIGHT,
		BTN_MIDDLE,
		NUM_BTNS,
	};
	int x;
	int y;
	int prevX;
	int prevY;
	int dx;
	int dy;
	void SetFromPBState(const PB_MouseData* state)
	{
		x = state->xPos; y = state->yPos; dx += state->dx; dy += state->dy;

		if (state->lPressed) butns[BTN_LEFT].Press();
		else if (state->lUp) butns[BTN_LEFT].Release();
		if (state->rPressed) butns[BTN_RIGHT].Press();
		else if(state->rUp) butns[BTN_RIGHT].Release();
		if (state->mPressed) butns[BTN_MIDDLE].Press();
		else if (state->mUp) butns[BTN_MIDDLE].Release();
	}
	void FrameEnd() {
		prevX = x; prevY = y;
		dx = 0; dy = 0;
		for (int i = 0; i < NUM_BTNS; i++) butns[i].FrameEnd();
	}
	InputButton butns[NUM_BTNS];
};


struct KeyboardState
{
	enum BUTTONS
	{
		BTN_F1, BTN_F2, BTN_F3, BTN_F4, BTN_F5, BTN_F6, BTN_F7, BTN_F8, BTN_F9, BTN_F10, BTN_F11, BTN_F12,

	};
};