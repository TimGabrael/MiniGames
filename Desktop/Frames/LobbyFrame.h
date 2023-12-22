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

	void SetTimer(float timer);

	static VoteData data;
	std::string pluginCache;



private:

	AdminWidget* adminWidget = nullptr;

    class QVBoxLayout* nameList = nullptr;
	QWidget* playerScrollContent = nullptr;
	ContentWidget* gamesContent = nullptr;
	float timerCache;
	
};
