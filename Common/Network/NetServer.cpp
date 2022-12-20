#include "NetServer.h"
#include "NetCommon.h"


static void* __stdcall JoinDeserializer(char* packet, int size)
{
	static base::ClientJoin join;
	if (join.ParseFromArray(packet, size)) return &join;
	return nullptr;
}
static void* __stdcall StateDeserializer(char* packet, int size)
{
	static base::ClientState state;
	if (state.ParseFromArray(packet, size)) return &state;
	return nullptr;
}


static NetServer* g_serverInstance = nullptr;
void NetServer::SteamNetServerConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetServer* server = g_serverInstance;
	if (!server) return;

	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		ServerConnection* conn = server->GetFromNetworkConnection(pInfo->m_hConn);
		if (conn)
		{
			server->CloseConnection(conn);
		}
		else
		{
			server->socket.networking->CloseConnection(pInfo->m_hConn, 17, "internal problems", false);
		}
		break;
	}

	case k_ESteamNetworkingConnectionState_Connecting:
	{
		ServerConnection* conn = server->GetNew();
		if (conn)
		{
			conn->conn = pInfo->m_hConn;
			conn->isConnected = ConnectionState::Connecting;
			conn->id = server->GetClientID(conn);
			conn->state = ClientState::INVALID;
			conn->name = "";
			conn->activePlugin = INVALID_ID;
			if (server->socket.networking->AcceptConnection(pInfo->m_hConn) != k_EResultOK)
			{
				server->CloseConnection(conn);
				return;
			}
			if (!server->socket.networking->SetConnectionPollGroup(pInfo->m_hConn, server->group))
			{
				server->CloseConnection(conn);
			}
			server->socket.networking->SetConnectionUserData(pInfo->m_hConn, (int64)conn);
		}
		else
		{
			server->socket.networking->CloseConnection(pInfo->m_hConn, 1, "Server is Full", false);
		}
		break;
	}

	case k_ESteamNetworkingConnectionState_None:
	case k_ESteamNetworkingConnectionState_Connected:
	default:
		break;
	}
}

NetServer* NetServer::Create(const char* ip, uint32_t port)
{
	uint32_t ipAddr = ParseIP(ip);

	NetServer* out = new NetServer();
	out->socket.networking = SteamNetworkingSockets();

	SteamNetworkingIPAddr serverLocalAddr;
	serverLocalAddr.Clear();
	serverLocalAddr.SetIPv4(ipAddr, port);

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetServerConnectionStatusChangedCallback);
	out->socket.socket = out->socket.networking->CreateListenSocketIP(serverLocalAddr, 1, &opt);
	if (out->socket.socket == k_HSteamListenSocket_Invalid) {
		delete out;
		return nullptr;
	}
	out->group = out->socket.networking->CreatePollGroup();
	if (out->group == k_HSteamNetPollGroup_Invalid) {
		delete out;
		return nullptr;
	}

	g_serverInstance = out;

	out->SetDeserializer(StateDeserializer, Client_State);
	out->SetDeserializer(JoinDeserializer, Client_Join);

	out->SetCallback((ServerPacketFunction)ClientStatePacketCallback, Client_State);
	out->SetCallback((ServerPacketFunction)ClientJoinPacketCallback, Client_Join);

	return out;
}

NetServer::~NetServer()
{

}

ServerConnection* NetServer::GetConnection(uint16_t id)
{
	if (id < MAX_PLAYERS)
	{
		if (socket.connected[id].isConnected == ConnectionState::Connected)
		{
			return &socket.connected[id];
		}
	}
	return nullptr;
}

void NetServer::SetCallback(ServerPacketFunction fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).receiver = fn;
}
void NetServer::SetDeserializer(DeserializationFunc fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).deserializer = fn;
}

void NetServer::SetJoinCallback(ServerJoinCallbackFunction fn)
{
	joinCB = fn;
}

void NetServer::SetDisconnectCallback(ServerDisconnectCallbackFunction fn)
{
	disconnectCB = fn;
}

bool NetServer::SendData(ServerConnection* conn, uint16_t packetID, const void* data, uint32_t size, uint32_t flags)
{
	if (tempStorage.size() < size + 10)
	{
		tempStorage.resize(size + 10);
	}
	*(uint16_t*)&tempStorage.at(0) = packetID;
	memcpy(&tempStorage.at(sizeof(packetID)), data, size);
	int64 outMsgNum = 0;
	EResult res = socket.networking->SendMessageToConnection(conn->conn, tempStorage.data(), size + sizeof(packetID), flags, &outMsgNum);
	return EResult::k_EResultOK == res;
}

bool NetServer::IsP2P() const
{
	return false;
}

void NetServer::Poll()
{
	if (group == k_HSteamNetPollGroup_Invalid) return;
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = socket.networking->ReceiveMessagesOnPollGroup(group, &pIncomingMsg, 1);

		if (numMsgs == 0)
			break;
		if (numMsgs < 0) return;

		uint32 msgSize = pIncomingMsg->GetSize();
		if (pIncomingMsg->GetSize() < sizeof(uint16_t))
		{
			pIncomingMsg->Release();
			continue;
		}
		const uint16_t header = *(uint16_t*)pIncomingMsg->GetData();

		if (header < callbacks.size())
		{
			CallbackInfo& info = callbacks.at(header);
			if (info.receiver)
			{
				void* packet = (void*)((uint16_t*)pIncomingMsg->GetData() + 1);
				if (info.deserializer) packet = info.deserializer((char*)packet, msgSize - sizeof(uint16_t));
				if (packet)
				{
					info.receiver(this, (ServerConnection*)pIncomingMsg->GetConnectionUserData(), packet, msgSize);
				}
			}
		}



		pIncomingMsg->Release();
	}
}

ServerConnection* NetServer::GetNew()
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (socket.connected[i].isConnected == ConnectionState::Disconnected)
		{
			return &socket.connected[i];
		}
	}
	return nullptr;
}
ServerConnection* NetServer::GetFromNetworkConnection(HSteamNetConnection conn)
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (socket.connected[i].isConnected != ConnectionState::Disconnected && socket.connected[i].conn == conn)
		{
			return &socket.connected[i];
		}
	}
	return nullptr;
}
void NetServer::CloseConnection(ServerConnection* c)
{
	if (c->isConnected != ConnectionState::Disconnected && c->conn != 0)
	{
		if (c->isConnected == ConnectionState::Connected)
		{
			base::ServerClientInfo info;
			info.set_is_connected(false);
			info.set_is_local(false);
			info.mutable_data()->set_name(c->name);
			info.mutable_data()->set_id(c->id);
			info.mutable_data()->set_is_admin(c->isAdmin);

			std::string serMsg = info.SerializeAsString();
			ServerConnection* firstAvailable = nullptr;
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* other = &socket.connected[i];
				if (other == c) continue;
				if (other->isConnected == ConnectionState::Connected && other->conn != k_HSteamNetConnection_Invalid)
				{
					SendData(other, Server_ClientInfo, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
					firstAvailable = other;
				}
			}

			if (c->isAdmin && firstAvailable)
			{
				firstAvailable->isAdmin = true;
				info.set_is_connected(true);
				info.set_is_local(false);
				info.mutable_data()->set_name(firstAvailable->name);
				info.mutable_data()->set_id(firstAvailable->id);
				info.mutable_data()->set_is_admin(true);

				std::string serAdmin = info.SerializeAsString();
				{
					info.set_is_local(true);
					std::string adminToAdmin = info.SerializeAsString();
					SendData(firstAvailable, Server_ClientInfo, adminToAdmin.data(), adminToAdmin.length(), SendFlags::Send_Reliable);
				}
				for (int i = 0; i < MAX_PLAYERS; i++)
				{
					ServerConnection* other = &socket.connected[i];
					if (other == c || other == firstAvailable) continue;
					if (other->isConnected == ConnectionState::Connected && other->conn != k_HSteamNetConnection_Invalid)
					{
						SendData(other, Server_ClientInfo, serAdmin.data(), serAdmin.length(), SendFlags::Send_Reliable);
					}
				}
			}

		}

		socket.networking->CloseConnection(c->conn, 1, nullptr, false);
		c->isAdmin = false;
		c->activePlugin = INVALID_ID;
		c->conn = 0;
		c->isConnected = ConnectionState::Disconnected;
		c->name = "";
		c->state = ClientState::INVALID;
	}
}
uint16_t NetServer::GetClientID(ServerConnection* conn)
{
	uintptr_t id = ((uintptr_t)conn - (uintptr_t)socket.connected) / sizeof(ServerConnection);
	if (id < 0xFFFF)
	{
		return id;
	}
	return INVALID_ID;
}
bool NetServer::ClientJoinPacketCallback(NetServer* s, ServerConnection* client, base::ClientJoin* join, int size)
{
	NameValidationResult res = ValidateName(join->name());
	if (res != NameValidationResult::Name_Ok)
	{
		s->CloseConnection(client);
	}
	else
	{
		client->name = join->name();
		client->isAdmin = !s->hasAdmin();
		client->isConnected = ConnectionState::Connected;
		client->id = s->GetClientID(client);
		client->activePlugin = INVALID_ID;
		
		// Send New Client Info To All Connected Clients
		{
			base::ServerClientInfo msg;

			msg.set_is_local(true);
			msg.set_is_connected(true);
			msg.mutable_data()->set_id(client->id);
			msg.mutable_data()->set_is_admin(client->isAdmin);
			msg.mutable_data()->set_name(client->name);

			std::string serMsg = msg.SerializeAsString();
			s->SendData(client, Server_ClientInfo, serMsg.data(), serMsg.size(), SendFlags::Send_Reliable);

			msg.set_is_local(false);
			serMsg = msg.SerializeAsString();
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->GetConnection(i);
				if (conn == client) continue;
				if (conn) {
					s->SendData(conn, Server_ClientInfo, serMsg.data(), serMsg.size(), SendFlags::Send_Reliable);

					base::ServerClientInfo otherConn;
					otherConn.mutable_data()->set_name(conn->name);
					otherConn.mutable_data()->set_id(conn->id);
					otherConn.mutable_data()->set_is_admin(conn->isAdmin);
					otherConn.set_is_local(false);
					otherConn.set_is_connected(true);
					std::string otherMsg = otherConn.SerializeAsString();
					s->SendData(client, Server_ClientInfo, otherMsg.data(), otherMsg.size(), SendFlags::Send_Reliable);

				}
			}
		}

		base::ServerSetState newState;
		newState.set_state((int32_t)ClientState::LOBBY);
		std::string serMsg = newState.SerializeAsString();
		s->SendData(client, Server_SetState, serMsg.data(), serMsg.length(), Send_Reliable);

	}

	join->Clear();
	return true;
}
bool NetServer::ClientStatePacketCallback(NetServer* s, ServerConnection* client, base::ClientState* state, int size)
{
	int32 stateVal = state->state();
	if (stateVal >= 0 && stateVal <= (int32)ClientState::PLUGIN)
	{
		client->state = (ClientState)stateVal;
		if (state->has_plugin_session_id())
		{
			client->activePlugin = state->plugin_session_id();
		}
		else
		{
			client->activePlugin = INVALID_ID;
		}
	}
	else
	{
		client->state = ClientState::INVALID;
		client->activePlugin = INVALID_ID;
	}
	
	state->Clear();
	return true;
}

bool NetServer::hasAdmin() const
{
	for (int i = 0; i < MAX_PLAYERS; i++)
	{
		if (socket.connected[i].isConnected == ConnectionState::Connected && socket.connected[i].isAdmin)
		{
			return true;
		}
	}
	return false;
}