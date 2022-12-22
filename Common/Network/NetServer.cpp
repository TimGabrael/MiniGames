#include "NetServer.h"
#include "NetCommon.h"
#include <unordered_map>


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
static void* __stdcall KickDeserializer(char* packet, int size)
{
	static base::ClientAdminKick kick;
	if (kick.ParseFromArray(packet, size)) return &kick;
	return nullptr;
}
static void* __stdcall LobbyAdminTimerDeserializer(char* packet, int size)
{
	static base::ClientLobbyAdminTimer timer;
	if (timer.ParseFromArray(packet, size)) return &timer;
	return nullptr;
}
static void* __stdcall LobbyVoteDeserializer(char* packet, int size)
{
	static base::ClientLobbyVote vote;
	if (vote.ParseFromArray(packet, size)) return &vote;
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
			conn->state = AppState::INVALID;
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

	out->SetDeserializer(KickDeserializer, Client_Adminkick);
	out->SetDeserializer(StateDeserializer, Client_State);
	out->SetDeserializer(JoinDeserializer, Client_Join);

	out->SetCallback((ServerPacketFunction)ClientAdminKickPacketCallback, Client_Adminkick);
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

void NetServer::SetClientStateCallback(ServerClientStateChangeCallbackFunction fn)
{
	stateCB = fn;
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
					info.receiver((NetServerInfo*)data, (ServerConnection*)pIncomingMsg->GetConnectionUserData(), packet, msgSize);
				}
			}
		}



		pIncomingMsg->Release();
	}
}

bool NetServer::CheckConnectionStateAndSend(ServerConnection* c)
{
	assert(data != nullptr);

	if (data->info.plugin)
	{
		return CheckConnectionStateAndSendInternal(c, AppState::PLUGIN, data->activePluingID);
	}
	else
	{
		return CheckConnectionStateAndSendInternal(c, AppState::LOBBY, 0xFFFF);
	}
	
	return false;
}

bool NetServer::CheckConnectionStateAndSendInternal(ServerConnection* c, AppState s, uint16_t pluginID)
{
	if (c->state == s)
	{
		if (s == AppState::LOBBY) return true;
		else if(s == AppState::PLUGIN) {
			return c->activePlugin == pluginID;
		}
	}
	base::ServerSetState stateChange;
	stateChange.set_state((int32_t)s);
	if (s == AppState::PLUGIN) stateChange.set_state(pluginID);

	std::string serMsg = stateChange.SerializeAsString();
	SendData(c, Server_SetState, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);

	return false;
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
			if (disconnectCB) disconnectCB((NetServerInfo*)data, c);

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
		c->state = AppState::INVALID;
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
bool NetServer::ClientJoinPacketCallback(ServerData* s, ServerConnection* client, base::ClientJoin* join, int size)
{
	NameValidationResult res = ValidateName(join->name());
	bool worked = true;;
	if (res != NameValidationResult::Name_Ok)
	{
		worked = false;
		s->info.net->CloseConnection(client);
	}
	else
	{
		client->name = join->name();
		client->isAdmin = !s->info.net->hasAdmin();
		client->isConnected = ConnectionState::Connected;
		client->id = s->info.net->GetClientID(client);
		client->activePlugin = INVALID_ID;
		NetResult joinRes = { "", true };
		if (s->info.net->joinCB)
		{
			joinRes = s->info.net->joinCB((NetServerInfo*)s, client);
			worked = joinRes.success;
			if(!worked) s->info.net->CloseConnection(client);
		}
	}

	if(worked)
	{

		// Send Available Plugins
		{
			base::ServerPlugin pluginData;
			pluginData.mutable_data()->set_id("a3fV-6giK-10Eb-2rdT");
			pluginData.mutable_data()->set_session_id(0);
			std::string serMsg = pluginData.SerializeAsString();
			s->info.net->SendData(client, Server_Plugin, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
		}

		// Send New Client Info To All Connected Clients
		{
			base::ServerClientInfo msg;

			msg.set_is_local(true);
			msg.set_is_connected(true);
			msg.mutable_data()->set_id(client->id);
			msg.mutable_data()->set_is_admin(client->isAdmin);
			msg.mutable_data()->set_name(client->name);

			std::string serMsg = msg.SerializeAsString();
			s->info.net->SendData(client, Server_ClientInfo, serMsg.data(), serMsg.size(), SendFlags::Send_Reliable);

			msg.set_is_local(false);
			serMsg = msg.SerializeAsString();
			for (int i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->info.net->GetConnection(i);
				if (conn == client) continue;
				if (conn) {
					s->info.net->SendData(conn, Server_ClientInfo, serMsg.data(), serMsg.size(), SendFlags::Send_Reliable);

					base::ServerClientInfo otherConn;
					otherConn.mutable_data()->set_name(conn->name);
					otherConn.mutable_data()->set_id(conn->id);
					otherConn.mutable_data()->set_is_admin(conn->isAdmin);
					otherConn.set_is_local(false);
					otherConn.set_is_connected(true);
					std::string otherMsg = otherConn.SerializeAsString();
					s->info.net->SendData(client, Server_ClientInfo, otherMsg.data(), otherMsg.size(), SendFlags::Send_Reliable);

				}
			}
		}

		// Send State Change
		{
			base::ServerSetState newState;
			newState.set_state((int32_t)AppState::LOBBY);
			std::string serMsg = newState.SerializeAsString();
			s->info.net->SendData(client, Server_SetState, serMsg.data(), serMsg.length(), Send_Reliable);
		}
	
	}

	join->Clear();
	return true;
}
bool NetServer::ClientStatePacketCallback(ServerData* s, ServerConnection* client, base::ClientState* state, int size)
{
	const int32 stateVal = state->state();
	const AppState oldAppState = client->state;
	const uint16_t oldPlugin = client->activePlugin;
	if (stateVal >= 0 && stateVal <= (int32)AppState::PLUGIN)
	{
		client->state = (AppState)stateVal;
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
		client->state = AppState::INVALID;
		client->activePlugin = INVALID_ID;
	}

	if (s->info.net->stateCB && ((oldAppState != client->state) || (oldPlugin != client->activePlugin)))
	{
		s->info.net->stateCB((NetServerInfo*)s, client);
	}
	
	state->Clear();
	return true;
}
bool NetServer::ClientAdminKickPacketCallback(ServerData* s, ServerConnection* client, base::ClientAdminKick* kick, int size)
{
	if (client->isAdmin)
	{
		ServerConnection* conn = s->info.net->GetConnection(kick->id());
		if (conn)
		{
			s->info.net->CloseConnection(conn);
		}
	}

	kick->Clear();
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


















static NetResult _stdcall LobbyJoinCallback(ServerData* s, ServerConnection* c)
{
	for (int i = 0; i < s->lobbyData.votes.size(); i++)
	{
		
		base::ServerLobbyVote vote;
		vote.set_client_id(s->lobbyData.votes.at(i).clientID);
		vote.set_plugin_id(s->lobbyData.votes.at(i).pluginID);

		const std::string serMsg = vote.SerializeAsString();
		s->info.net->SendData(c, Server_LobbyVote, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
	}


	NetResult res {"", true};
	return res;
}
static void __stdcall LobbyDisconnectCallback(ServerData* s, ServerConnection* c)
{
	for (int i = 0; i < s->lobbyData.votes.size(); i++)
	{
		if (s->lobbyData.votes.at(i).clientID == c->id)
		{
			base::ServerLobbyVote vote;
			vote.set_client_id(c->id);
			vote.set_plugin_id(INVALID_ID);
			s->lobbyData.votes.erase(s->lobbyData.votes.begin() + i);
			
			const std::string serMsg = vote.SerializeAsString();
			for (uint16_t j = 0; j < MAX_PLAYERS; j++)
			{
				ServerConnection* conn = s->info.net->GetConnection(j);
				if (conn && conn != c)
				{
					s->info.net->SendData(conn, Server_LobbyVote, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
				}
			}
			i--;
		}
	}


}

static bool __stdcall ClientLobbyVotePacketCallback(ServerData* s, ServerConnection* client, base::ClientLobbyVote* vote, int size)
{
	if (s->info.net->CheckConnectionStateAndSend(client))
	{
		assert(server != nullptr);

		std::string serMsg;
		ServerData::ClientVoteData data;
		bool isValid = true;
		{
			uint32_t plID = vote->plugin_id();
			base::ServerLobbyVote response;

			response.set_client_id(client->id);
			data.clientID = client->id;
			if (plID == 0 || plID == INVALID_ID)	// currently only 0 and invalid plugin supported
			{
				data.pluginID = plID;
				response.set_plugin_id(plID);

			}
			else
			{
				data.pluginID = INVALID_ID;
				response.set_plugin_id(INVALID_ID);
				isValid = false;
			}

			serMsg = response.SerializeAsString();
		}

		ServerData::ClientVoteData* found = nullptr;
		for (ServerData::ClientVoteData& v : s->lobbyData.votes)
		{
			if (v.clientID == data.clientID)
			{
				found = &v;
				break;
			}
		}

		if (found)
		{
			if (found->pluginID == data.pluginID) {
				return true; // don't do anything if the state did not change
			}
			found->pluginID = data.pluginID;
		}
		else
		{
			s->lobbyData.votes.push_back(data);
		}

		if (isValid)
		{
			// send to other clients
			for (uint16_t i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->info.net->GetConnection(i);
				if (conn && conn != client && s->info.net->CheckConnectionStateAndSend(conn))
				{
					s->info.net->SendData(conn, Server_LobbyVote, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
				}
			}
		}
		else
		{
			// send to all clients
			for (uint16_t i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->info.net->GetConnection(i);
				if (conn && s->info.net->CheckConnectionStateAndSend(conn))
				{
					s->info.net->SendData(conn, Server_LobbyVote, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
				}
			}
		}


	}
	vote->Clear();
	return true;
}
static bool __stdcall ClientLobbyAdminTimerPacketCallback(ServerData* s, ServerConnection* client, base::ClientLobbyAdminTimer* timer, int size)
{
	if (s->info.net->CheckConnectionStateAndSend(client) && client->isAdmin)
	{
		assert(serverData != nullptr);

		float time = std::min(timer->time(), 120.0f);// max time 120.0f
		if (time > 0.0f)
		{
			s->lobbyData.timerRunning = true;
			s->lobbyData.timer = time;
			base::ServerLobbyTimer response;
			response.set_time(time);
			const std::string serMsg = response.SerializeAsString();
			for (uint16_t i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->info.net->GetConnection(i);
				if (conn && s->info.net->CheckConnectionStateAndSend(conn))
				{
					s->info.net->SendData(conn, Server_LobbyTimer, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
				}
			}
		}
		else
		{
			std::string serMsg;
			if (s->lobbyData.votes.size() > 0)
			{
				// Get most voted plugin ID
				std::unordered_map<uint16_t, uint16_t> votes;
				for (int i = 0; i < s->lobbyData.votes.size(); i++)
				{
					votes[s->lobbyData.votes.at(i).pluginID]++;
				}
				auto val = std::max_element(votes.begin(), votes.end(), [](const std::pair<uint16_t, uint16_t>& f, const std::pair<uint16_t, uint16_t>& s) {
					return f.second < s.second;
				});
				
				printf("Most voted Plugin/numVotes: %d %d\n", val->first, val->second);

				base::ServerSetState state;
				state.set_plugin_id(val->first);
				state.set_state((int32_t)AppState::PLUGIN);
				serMsg = state.SerializeAsString();
			}
			else
			{
				
				base::ServerSetState state;
				state.set_plugin_id(0);
				state.set_state((int32_t)AppState::PLUGIN);
				serMsg = state.SerializeAsString();
			}

			for (uint16_t i = 0; i < MAX_PLAYERS; i++)
			{
				ServerConnection* conn = s->info.net->GetConnection(i);
				if (conn)
				{
					s->info.net->SendData(conn, Server_SetState, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
				}
			}

		}
	}
	timer->Clear();
	return true;
}
static void __stdcall LobbyClientStateChangeCallback(ServerData* s, ServerConnection* client)
{
	if (s->info.net->CheckConnectionStateAndSend(client))
	{
		assert(serverData != nullptr);

		for (const auto& v : s->lobbyData.votes)
		{
			base::ServerLobbyVote vote;
			vote.set_client_id(v.clientID);
			vote.set_plugin_id(v.pluginID);
			const std::string serMsg = vote.SerializeAsString();
			s->info.net->SendData(client, Server_LobbyVote, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
		}

		if (s->lobbyData.timerRunning)
		{
			base::ServerLobbyTimer timer;
			timer.set_time(s->lobbyData.timer);
			const std::string serMsg = timer.SerializeAsString();
			s->info.net->SendData(client, Server_LobbyTimer, serMsg.data(), serMsg.length(), SendFlags::Send_Reliable);
		}

	}
}
ServerData::ServerData(const char* ip, uint32_t port)
{
	info.net = NetServer::Create(ip, port);
	info.net->data = this;
	SetLobbyState();
}
ServerData::~ServerData()
{
	
}
void ServerData::SetLobbyState()
{
	info.net->SetDeserializer(LobbyVoteDeserializer, Client_LobbyVote);
	info.net->SetDeserializer(LobbyAdminTimerDeserializer, Client_LobbyAdminTimer);

	info.net->SetCallback((ServerPacketFunction)ClientLobbyVotePacketCallback, Client_LobbyVote);
	info.net->SetCallback((ServerPacketFunction)ClientLobbyAdminTimerPacketCallback, Client_LobbyAdminTimer);

	info.net->SetClientStateCallback((ServerClientStateChangeCallbackFunction)LobbyClientStateChangeCallback);
	info.net->SetJoinCallback((ServerJoinCallbackFunction)LobbyJoinCallback);
	info.net->SetDisconnectCallback((ServerDisconnectCallbackFunction)LobbyDisconnectCallback);
}
void ServerData::Update(float dt)
{
	info.net->Poll();
	NetRunCallbacks();
}