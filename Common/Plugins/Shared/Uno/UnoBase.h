#pragma once
#include <stdint.h>
#include "UnoMessages.pb.h"

#define UNO_PLUGIN_ID "a3fV-6giK-10Eb-2rdT"

enum CardFace : uint8_t
{
	CARD_VAL_0,
	CARD_VAL_1,
	CARD_VAL_2,
	CARD_VAL_3,
	CARD_VAL_4,
	CARD_VAL_5,
	CARD_VAL_6,
	CARD_VAL_7,
	CARD_VAL_8,
	CARD_VAL_9,
	CARD_PAUSE,
	CARD_FLIP,
	CARD_PULL_2,
	CARD_CHOOSE_COLOR,
	CARD_PULL_4,

	CARD_UNKNOWN = 0xFF,
};

enum CardColor : uint8_t
{
	CARD_COLOR_RED,
	CARD_COLOR_YELLOW,
	CARD_COLOR_GREEN,
	CARD_COLOR_BLUE,
	CARD_COLOR_BLACK,

	CARD_COLOR_UNKOWN = 0xFF,
};


struct CardData
{
	CardData(CardFace f, CardColor c) : face(f), color(c) {}
	CardFace face;
	CardColor color;
};


enum ClientUnoMessages
{
	Client_UnoPlayCard = NUM_CLIENT_BASE_MESSAGES,
	Client_UnoPullCards,
};
enum ServerUnoMessages
{
	Server_UnoPullCards = NUM_SERVER_BASE_MESSAGES,
	Server_UnoPlayCard,
};