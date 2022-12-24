#include "Uno.h"

SERVER_PLUGIN_EXPORT_DEFINITION(Uno);


#define CARDS_PULLED_AT_START 10

static void __stdcall UnoStateChangeCallback(NetServerInfo* info, ServerConnection* client)
{
	if (info->net->CheckConnectionStateAndSend(client))
	{
		Uno* uno = (Uno*)info->plugin;
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

			const std::string serMsg = cards.SerializeAsString();
			info->net->SendData(client, Server_UnoPullCards, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
		}
		// Set the top card
		{
			uno::ServerPlayCard play;
			play.set_client_id(0xFFFF);
			play.set_next_player_in_turn(uno->data->playerInTurn);
			play.mutable_card()->set_face(uno->data->topCard.face);
			play.mutable_card()->set_color(uno->data->topCard.color);

			const std::string serMsg = play.SerializeAsString();
			info->net->SendData(client, Server_UnoPlayCard, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
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
	if (info->net->CheckConnectionStateAndSend(client) && uno->data->playerInTurn == client->id)
	{
		uint32_t face = play->card().face();
		uint32_t col = play->card().color();
		if (face < CardFace::CARD_PULL_4 + 1 && col < CardColor::CARD_COLOR_BLACK + 1)
		{
			for (auto& p : uno->data->playerData)
			{
				if (p.clientID != client->id) continue;

				for (int i = 0; i < p.cardHand.cards.size(); i++)
				{
					const CardData& c = p.cardHand.cards.at(i);
					if (c.face != face) continue;

					if (c.color == CARD_COLOR_BLACK && (col == CARD_COLOR_BLACK || col == CARD_COLOR_UNKOWN))
					{
						continue;
					}
					else if(c.color != col)
					{
						continue;
					}

					if (IsCardPlayable(uno->data->topCard, c))
					{
						uno->data->topCard = c;
						uno->data->playerInTurn = (uno->data->playerInTurn + 1) % uno->data->playerData.size();
						uno::ServerPlayCard play;
						play.set_client_id(client->id);
						play.mutable_card()->set_face(face);
						play.mutable_card()->set_color(col);
						play.set_next_player_in_turn(uno->data->playerInTurn);
						const std::string serMsg = play.SerializeAsString();

						for (uint16_t j = 0; j < MAX_PLAYERS; j++)
						{
							ServerConnection* conn = info->net->GetConnection(j);
							if (conn && info->net->CheckConnectionStateAndSend(conn))
							{
								info->net->SendData(conn, Server_UnoPlayCard, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
							}
						}

						p.cardHand.cards.erase(p.cardHand.cards.begin() + i);
					}
					return true;
					
				}
			
			}
		}
	}
	return true;
}
static bool __stdcall UnoPullCardsCallback(NetServerInfo* info, ServerConnection* client, void* pull, int packetSize)
{
	Uno* uno = (Uno*)info->plugin;
	if (uno->data->playerInTurn == client->id)
	{
		std::cout << "client pulling cards" << std::endl;
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
	data->playerInTurn = 0;

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
