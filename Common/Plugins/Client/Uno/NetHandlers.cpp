#include "NetHandlers.h"
#include "../../Shared/Uno/UnoBase.h"
#include "Uno.h"

static void* __stdcall PlayCardDeserializer(char* packet, int packetSize)
{
	static uno::ServerPlayCard play;
	if (play.ParseFromArray(packet, packetSize))  return &play;
	return nullptr;
}
static void* __stdcall PullCardsDeserializer(char* packet, int packetSize)
{
	static uno::ServerPullCards pull;
	if (pull.ParseFromArray(packet, packetSize)) return &pull;
	return nullptr;
}
static void* __stdcall ResyncDeserializer(char* packet, int packetSize) {
    static uno::ServerResync sync;
    if(sync.ParseFromArray(packet, packetSize)) return &sync;
    return nullptr;
}
static void* __stdcall FinishedDeserializer(char* packet, int packetSize) {
    static uno::ServerGameFinished fin;
    if(fin.ParseFromArray(packet, packetSize)) return &fin;
    return nullptr;
}

static bool __stdcall PlayCardCallback(ApplicationData* app, uno::ServerPlayCard* play, int packetSize)
{
	UnoPlugin* uno = GetInstance();
	GameStateData* game = GetGameState();
	uint16_t cID = play->client_id();
	uint32_t face = play->card().face();
	uint32_t color = play->card().color();
	if (face <= CardFace::CARD_UNKNOWN && color <= CardColor::CARD_COLOR_UNKOWN)
	{
		CardData playing((CardFace)face, (CardColor)color);
		CARD_ID outCard = CARD_ID::NUM_AVAILABLE_CARDS;
		COLOR_ID outColor = COLOR_INVALID;
		CardDataToRenderCard(playing, outCard, outColor);
		if (cID == 0xFFFF)
		{
			// server plays the card
			uno->g_objs->stack.SetTop(outCard, outColor);
		}
		else
		{
			CardHand* hand = game->GetHandForcefully(cID);
			hand->PlayCardServer(uno->g_objs->stack, uno->g_objs->anims, outCard, outColor);
		}
		game->playerInTurn = play->next_player_in_turn();
		game->topCard = playing;
        uno->g_objs->stack.blackColorID = (COLOR_ID)playing.color;
	}
	return true;
}
static bool __stdcall PullCardCallback(ApplicationData* app, uno::ServerPullCards* pull, int packetSize)
{
	UnoPlugin* uno = GetInstance();
	GameStateData* game = GetGameState();
	
	for (const auto& p : pull->pulls())
	{
		PlayerInfo* info = game->GetPlayerInfoForcefully(p.client_id());
		if (!info->hand) info->hand = game->GetHandForcefully(p.client_id());

		for (const auto& c : p.cards())
		{
			CardData card((CardFace)c.face(), (CardColor)c.color());
			CARD_ID addCard = CARD_ID_BLANK;
			COLOR_ID temp = COLOR_INVALID;
			CardDataToRenderCard(card, addCard, temp);
			info->hand->PullCardServer(uno->g_objs->playerCam, uno->g_objs->stack, uno->g_objs->deck, uno->g_objs->anims, addCard);
		}
	}
	game->playerInTurn = pull->next_player_in_turn();
	return true;
}
static bool __stdcall ResyncCallback(ApplicationData* app, uno::ServerResync* sync, int packetSize) {
    UnoPlugin* uno = GetInstance();
    GameStateData* data = GetGameState();
    for(const auto& cl : sync->sync()) {
        PlayerInfo* info = data->GetPlayerInfoForcefully(cl.client_id());
        // todo: change all cards of the current player
    }
    return true;
}
static bool __stdcall FinishedCallback(ApplicationData* app, uno::ServerGameFinished* sync, int packetSize) {
    UnoPlugin* uno = GetInstance();
    GameStateData* data = GetGameState();
    data->finished = true;
    // todo: let the admin restart the game or quit
    return true;
}


void UnoFillNetHandlers(struct ApplicationData *data) {

    data->net->SetDeserializer(PlayCardDeserializer, Server_UnoPlayCard);
    data->net->SetDeserializer(PullCardsDeserializer, Server_UnoPullCards);
    data->net->SetDeserializer(ResyncDeserializer, Server_UnoResync);
    data->net->SetDeserializer(FinishedDeserializer, Server_UnoFinished);
    data->net->SetCallback((ClientPacketFunction)PlayCardCallback, Server_UnoPlayCard);
    data->net->SetCallback((ClientPacketFunction)PullCardCallback, Server_UnoPullCards);
    data->net->SetCallback((ClientPacketFunction)ResyncCallback,  Server_UnoResync);
    data->net->SetCallback((ClientPacketFunction)FinishedCallback, Server_UnoFinished);

}

