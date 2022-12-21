#pragma once
#include "StateFrame.h"

struct VoteInfo
{
	uint16_t clientID;
	uint16_t pluginID;
};
struct VoteData
{
	std::vector<VoteInfo> votes;
	int remainingTime;
	bool isRunning = false;
};

class ContentWidget;
class AdminWidget;
class LobbyFrame : public StateFrame
{
public:
	LobbyFrame(QMainWindow* parent);
	~LobbyFrame();

	void StartPlugin();

	void Rebuild();
	void UpdateFromData();

	static VoteData data;
	std::string pluginCache;
private:


	AdminWidget* adminWidget = nullptr;

	void AddPlayer(const std::string& name);
	void RemovePlayer(const std::string& name);
	void ReSync();
	QWidget* playerScrollContent = nullptr;
	ContentWidget* gamesContent = nullptr;
	
};