#pragma once
#include "../ServerPlugin.h"
#include "Plugins/Shared/Uno/UnoBase.h"



struct Hand
{
	std::vector<CardData> cards;
};

struct PlayerData
{
	std::string name;
	Hand cardHand;
};

struct UnoData
{
	std::vector<PlayerData> playerData;

};


struct Uno : public ServerPlugin
{
	virtual ~Uno() {};
	virtual void Initialize(NetServerInfo* s) override;
	virtual void Update(float dt) override;
	virtual SERVER_PLUGIN_INFO GetInfo() const override;

	virtual void CleanUp() override;

	UnoData* data = nullptr;
};