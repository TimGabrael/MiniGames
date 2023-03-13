#pragma once


namespace QImGui {

	void* initialize(class QWidget* window, bool defaultRender = true);
	void newFrame(void* ref = nullptr);
	void render(void* ref = nullptr);
    void release(void* ref = nullptr);

}
