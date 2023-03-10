#pragma once
#include "../ServerPlugin.h"
#include "Plugins/Shared/Uno/UnoBase.h"
#include <algorithm>
#include <random>

#define CARDS_PULLED_AT_START 10

#define CARD_ROW1(COLOR)CardData(CardFace::CARD_VAL_0, COLOR), CardData(CardFace::CARD_VAL_1, COLOR),\
						CardData(CardFace::CARD_VAL_2, COLOR), CardData(CardFace::CARD_VAL_3, COLOR),\
						CardData(CardFace::CARD_VAL_4, COLOR), CardData(CardFace::CARD_VAL_5, COLOR),\
						CardData(CardFace::CARD_VAL_6, COLOR), CardData(CardFace::CARD_VAL_7, COLOR),\
						CardData(CardFace::CARD_VAL_8, COLOR), CardData(CardFace::CARD_VAL_9, COLOR),\
						CardData(CardFace::CARD_PAUSE, COLOR), CardData(CardFace::CARD_FLIP, COLOR),\
						CardData(CardFace::CARD_PULL_2, COLOR)

#define CARD_ROW2(COLOR)CardData(CardFace::CARD_VAL_1, COLOR),\
						CardData(CardFace::CARD_VAL_2, COLOR), CardData(CardFace::CARD_VAL_3, COLOR),\
						CardData(CardFace::CARD_VAL_4, COLOR), CardData(CardFace::CARD_VAL_5, COLOR),\
						CardData(CardFace::CARD_VAL_6, COLOR), CardData(CardFace::CARD_VAL_7, COLOR),\
						CardData(CardFace::CARD_VAL_8, COLOR), CardData(CardFace::CARD_VAL_9, COLOR),\
						CardData(CardFace::CARD_PAUSE, COLOR), CardData(CardFace::CARD_FLIP, COLOR),\
						CardData(CardFace::CARD_PULL_2, COLOR)

struct Deck
{
	Deck()
		: cards{
			CARD_ROW1(CARD_COLOR_RED), CARD_ROW1(CARD_COLOR_YELLOW), CARD_ROW1(CARD_COLOR_GREEN), CARD_ROW1(CARD_COLOR_BLUE),
			CARD_ROW2(CARD_COLOR_RED), CARD_ROW2(CARD_COLOR_YELLOW), CARD_ROW2(CARD_COLOR_GREEN), CARD_ROW2(CARD_COLOR_BLUE),
			CardData(CARD_CHOOSE_COLOR, CARD_COLOR_BLACK),CardData(CARD_CHOOSE_COLOR, CARD_COLOR_BLACK),CardData(CARD_CHOOSE_COLOR, CARD_COLOR_BLACK),CardData(CARD_CHOOSE_COLOR, CARD_COLOR_BLACK),
			CardData(CARD_PULL_4, CARD_COLOR_BLACK),CardData(CARD_PULL_4, CARD_COLOR_BLACK),CardData(CARD_PULL_4, CARD_COLOR_BLACK),CardData(CARD_PULL_4, CARD_COLOR_BLACK),
		}
	{
		Shuffle();
		curPullIdx = 0;
	}

	void Shuffle()
	{
		auto rng = std::default_random_engine{};
		std::shuffle(std::begin(cards), std::end(cards), rng);
	}

	CardData PullCard()
	{
		if (curPullIdx < 108)
		{
			CardData out = cards[curPullIdx];
			curPullIdx++;
			return out;
		}
		else
		{
			Shuffle();
			curPullIdx = 0;
			return PullCard();
		}
	}

private:

	CardData cards[108];
	uint8_t curPullIdx;
};




struct Hand
{
	std::vector<CardData> cards;
};

struct PlayerData
{
	std::string name;
	Hand cardHand;
	uint16_t clientID;
};



struct UnoData
{
	UnoData() : topCard(CARD_UNKNOWN, CARD_COLOR_UNKOWN) {}
	Deck cardDeck;
	std::vector<PlayerData> playerData;
	CardData topCard;
	uint16_t playerInTurnIdx = 0xFFFF;
	uint16_t accumulatedPull = 0;
	bool forward = true;
    bool cur_made_action = false;
};


struct Uno : public ServerPlugin
{
	virtual ~Uno() {};
	virtual void Initialize(NetServerInfo* s) override;
	virtual void Update(float dt) override;
	virtual SERVER_PLUGIN_INFO GetInfo() const override;

	virtual void CleanUp() override;

	UnoData* data = nullptr;
    NetServerInfo* net = nullptr;
};

SERVER_PLUGIN_EXPORTS();
