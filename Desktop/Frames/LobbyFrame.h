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


	static VoteData data;
	std::string pluginCache;
private:

	void UpdateFromData();

	AdminWidget* adminWidget = nullptr;

	void AddPlayer(const std::string& name);
	void RemovePlayer(const std::string& name);
	void ReSync();
	QWidget* playerScrollContent = nullptr;
	ContentWidget* gamesContent = nullptr;
	
};