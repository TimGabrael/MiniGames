#include "Uno.h"
#include "Plugins/Shared/Uno/UnoBase.h"
#include <stdio.h>

SERVER_PLUGIN_EXPORT_DEFINITION(Uno);


#define CARDS_PULLED_AT_START 10

static uint16_t UnoGetCurrentPlayerID(Uno* uno)
{
	if (uno->data->playerInTurnIdx < uno->data->playerData.size()) return uno->data->playerData.at(uno->data->playerInTurnIdx).clientID;
	return 0xFFFF;
}

static void UnoNextPlayer(Uno* uno)
{
	if (uno->data->forward) uno->data->playerInTurnIdx = (uno->data->playerInTurnIdx + 1) % uno->data->playerData.size();
	else uno->data->playerInTurnIdx = (uno->data->playerInTurnIdx - 1) % uno->data->playerData.size();
}
static void UnoSkipPlayer(Uno* uno)
{
	if (uno->data->forward) uno->data->playerInTurnIdx = (uno->data->playerInTurnIdx + 2) % uno->data->playerData.size();
	else uno->data->playerInTurnIdx = (uno->data->playerInTurnIdx - 2) % uno->data->playerData.size();
}

static void __stdcall UnoStateChangeCallback(NetServerInfo* info, ServerConnection* client)
{
	if (info->net->CheckConnectionStateAndSend(client))
	{
		Uno* uno = (Uno*)info->plugin;

		const uint16_t playerID = UnoGetCurrentPlayerID(uno);

		// tell about all cards
		{
			uno::ServerPullCards cards;
			for (int i = 0; i < uno->data->playerData.size(); i++)
			{
				PlayerData& p = uno->data->playerData.at(i);
				uno::PullData* pull = cards.mutable_pulls()->Add();
				pull->set_client_id(p.clientID);
				for (int j = 0; j < p.cardHand.cards.size(); j++)
				{

					uno::CardData* card = pull->mutable_cards()->Add();
					if (client->id == p.clientID)
					{
						const CardData& c = p.cardHand.cards.at(j);
						card->set_color(c.color);
						card->set_face(c.face);
					}
					else
					{
						card->set_color(CardColor::CARD_COLOR_UNKOWN);
						card->set_face(CardFace::CARD_UNKNOWN);
					}
				}
			}
			cards.set_next_player_in_turn(playerID);
			const std::string serMsg = cards.SerializeAsString();
            if(info->net->SerializeAndStore(Server_UnoPullCards, &cards)) {
                info->net->SendData(client, SendFlags::Send_Reliable);
            }
		}
		// Set the top card
		{
			uno::ServerPlayCard play;
			play.set_client_id(0xFFFF);
			play.set_next_player_in_turn(playerID);
			play.mutable_card()->set_face(uno->data->topCard.face);
			play.mutable_card()->set_color(uno->data->topCard.color);

            if(info->net->SerializeAndStore(Server_UnoPlayCard, &play)) {
                info->net->SendData(client, SendFlags::Send_Reliable);
            }
		}
	}
}
static void* __stdcall UnoPlayCardDeserializer(char* packet, int packetSize)
{
	static uno::ClientPlayCard card;
	if (card.ParseFromArray(packet, packetSize)) return &card;
	return nullptr;
}
static bool __stdcall UnoPlayCardCallback(NetServerInfo* info, ServerConnection* client, uno::ClientPlayCard* play, int packetSize)
{
	Uno* uno = (Uno*)info->plugin;
	const uint16_t turnID = UnoGetCurrentPlayerID(uno);
	if (info->net->CheckConnectionStateAndSend(client) && turnID == client->id)
	{
		uint32_t face = play->card().face();
		uint32_t col = play->card().color();
        if(col == CARD_COLOR_BLACK || col == CARD_COLOR_UNKOWN) return true;
		if (face < CardFace::CARD_PULL_4 + 1 && col < CardColor::CARD_COLOR_BLACK + 1)
		{
			for (auto& p : uno->data->playerData)
			{
				if (p.clientID != client->id) continue;

				for (int i = 0; i < p.cardHand.cards.size(); i++)
				{
					const CardData& c = p.cardHand.cards.at(i);
					if (c.face != face) continue;

					if(c.color != col)
					{
                        if(face != CARD_CHOOSE_COLOR && face != CARD_PULL_4) {
                            continue;
                        }
					}

					if (IsCardPlayable(uno->data->topCard, c))
					{
						uno->data->topCard.color = (CardColor)col;
                        uno->data->topCard.face = (CardFace)face;

						if (c.face == CARD_FLIP) uno->data->forward = !uno->data->forward;
						else if (c.face == CardFace::CARD_PULL_2) uno->data->accumulatedPull += 2;


						if (c.face == CARD_PAUSE) UnoSkipPlayer(uno);
						else UnoNextPlayer(uno);


						const uint16_t nextID = UnoGetCurrentPlayerID(uno);

						uno::ServerPlayCard play;
						play.set_client_id(client->id);
						play.mutable_card()->set_face(face);
						play.mutable_card()->set_color(col);
						play.set_next_player_in_turn(nextID);
                        if(info->net->SerializeAndStore(Server_UnoPlayCard, &play)) {
                            for (uint16_t j = 0; j < MAX_PLAYERS; j++)
                            {
                                ServerConnection* conn = info->net->GetConnection(j);
                                if (conn && info->net->CheckConnectionStateAndSend(conn))
                                {
                                    info->net->SendData(conn, SendFlags::Send_Reliable);
                                }
                            }
                        }
                        p.cardHand.cards.erase(p.cardHand.cards.begin() + i);
					}
					return true;
					
				}
			
			}
		}
		else if (face == CardFace::CARD_UNKNOWN && CardColor::CARD_COLOR_UNKOWN)
		{
			// TODO: Check if player is allowed to skip the turn

			UnoNextPlayer(uno);
			const uint16_t nextID = UnoGetCurrentPlayerID(uno);

			uno::ServerPullCards skip;
			skip.set_next_player_in_turn(nextID);
			const std::string serMsg = skip.SerializeAsString();
            if(info->net->SerializeAndStore(Server_UnoPullCards, &skip)) {
                for (uint16_t j = 0; j < MAX_PLAYERS; j++)
                {
                    ServerConnection* conn = info->net->GetConnection(j);
                    if (conn && info->net->CheckConnectionStateAndSend(conn))
                    {
                        info->net->SendData(conn, SendFlags::Send_Reliable);
                    }
                }
            }

		}
	}
	return true;
}
static bool __stdcall UnoPullCardsCallback(NetServerInfo* info, ServerConnection* client, void* pull, int packetSize)
{
	Uno* uno = (Uno*)info->plugin;
	const uint16_t turnIdx = uno->data->playerInTurnIdx;
	const uint16_t turnID = UnoGetCurrentPlayerID(uno);
	if (info->net->CheckConnectionStateAndSend(client) && turnID == client->id)
	{
		const int numCardsToBePulled = uno->data->accumulatedPull > 0 ? uno->data->accumulatedPull : 1;
		uno->data->accumulatedPull = 0;

		UnoNextPlayer(uno);
		const uint16_t nextID = UnoGetCurrentPlayerID(uno);

		PlayerData* turnPlayer = &uno->data->playerData.at(turnIdx);

		// pull cards for real and send to the current connection
		{
			uno::ServerPullCards pull;
			pull.set_next_player_in_turn(nextID);
			uno::PullData* p = pull.add_pulls();
			p->set_client_id(client->id);
			for (int i = 0; i < numCardsToBePulled; i++)
			{
				uno::CardData* card = p->add_cards();
				CardData pulled = uno->data->cardDeck.PullCard();
				card->set_color(pulled.color);
				card->set_face(pulled.face);
				turnPlayer->cardHand.cards.push_back(pulled);
			}
            if(info->net->SerializeAndStore(Server_UnoPullCards, &pull)) {
                info->net->SendData(client, SendFlags::Send_Reliable);
            }
		}

		// tell the others the player pulled "some" cards
		{
			uno::ServerPullCards pull;
			pull.set_next_player_in_turn(nextID);
			uno::PullData* p = pull.add_pulls();
			p->set_client_id(client->id);
			for (int i = 0; i < numCardsToBePulled; i++)
			{
				uno::CardData* card = p->add_cards();
				card->set_color(CARD_COLOR_UNKOWN);
				card->set_face(CARD_UNKNOWN);
			}
            if(info->net->SerializeAndStore(Server_UnoPullCards, &pull)) {
                for (uint16_t i = 0; i < MAX_PLAYERS; i++)
                {
                    ServerConnection* conn = info->net->GetConnection(i);
                    if (conn && conn != client && info->net->CheckConnectionStateAndSend(conn))
                    {
                        info->net->SendData(conn, SendFlags::Send_Reliable);
                    }
                }
            }

		}

	}
	return true;
}


void Uno::Initialize(NetServerInfo* s)
{
	this->data = new UnoData();

	s->net->SetClientStateCallback((ServerClientStateChangeCallbackFunction)UnoStateChangeCallback);
	s->net->SetDeserializer(UnoPlayCardDeserializer, Client_UnoPlayCard);
	s->net->SetDeserializer(nullptr, Client_UnoPullCards);
	s->net->SetCallback((ServerPacketFunction)UnoPlayCardCallback, Client_UnoPlayCard);
	s->net->SetCallback((ServerPacketFunction)UnoPullCardsCallback, Client_UnoPullCards);

	for (uint16_t i = 0; i < MAX_PLAYERS; i++)
	{
		ServerConnection* conn = s->net->GetConnection(i);
		if (conn)
		{
			PlayerData player;
			player.clientID = conn->id;
			player.name = conn->name;
			for (int i = 0; i < CARDS_PULLED_AT_START; i++)
			{
				player.cardHand.cards.push_back(data->cardDeck.PullCard());
			}
			data->playerData.push_back(std::move(player));
		}
	}
	data->topCard = data->cardDeck.PullCard();
	data->playerInTurnIdx = 0;

}

void Uno::Update(float dt)
{

}

SERVER_PLUGIN_INFO Uno::GetInfo() const
{
	static SERVER_PLUGIN_INFO info;
	memcpy(info.ID, UNO_PLUGIN_ID, sizeof(info.ID));
	info.updateTimer = 16.0f;
	info.allowLateJoin = false;
	return info;
}

void Uno::CleanUp()
{
}
