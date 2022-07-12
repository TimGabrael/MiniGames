#pragma once
#include "StateFrame.h"

class ContentWidget;
class LobbyFrame : public StateFrame
{
public:
	LobbyFrame(QMainWindow* parent);
	~LobbyFrame();

	void StartPlugin(int idx);



	virtual void FetchSyncData(std::string& str) override;
	virtual void HandleAddClient(const ClientData* added) override;
	virtual void HandleRemovedClient(const ClientData* removed) override;

	virtual void HandleNetworkMessage(Packet* packet) override;
	virtual void HandleSync(const std::string& syncData) override;

private:
	void AddPlayer(const std::string& name);
	void RemovePlayer(const std::string& name);
	void ReSync();
	QWidget* playerScrollContent = nullptr;
	ContentWidget* gamesContent = nullptr;
	
};