#pragma once

#include "StateFrame.h"


class SettingsFrame : public StateFrame
{
public:
	SettingsFrame(QMainWindow* parent);
	~SettingsFrame();

	void Create();


	virtual void FetchSyncData(std::string& str);
	virtual void HandleAddClient(const ClientData* added);
	virtual void HandleRemovedClient(const ClientData* removed);

	virtual void HandleNetworkMessage(Packet* packet);
	virtual void HandleSync(const std::string& syncData);


private:
	
	void OnFullSceen();
	
	
	struct CustomCheckBox* fullscreen;
};