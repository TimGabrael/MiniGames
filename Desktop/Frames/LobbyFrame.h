#pragma once
#include "StateFrame.h"

struct VoteInfo
{
	std::string pluginID;
	std::string username;
};
struct VoteData
{
	std::vector<VoteInfo> votes;
	float remainingTime;
};

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

	static bool VoteDataHandleNetworkMessage(Packet* packet);

	static VoteData data;
private:
	void AddPlayer(const std::string& name);
	void RemovePlayer(const std::string& name);
	void ReSync();
	QWidget* playerScrollContent = nullptr;
	ContentWidget* gamesContent = nullptr;
	
};