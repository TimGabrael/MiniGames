#include "NetClient.h"
#include "NetCommon.h"
#include "Network/NetworkBase.h"
#include "Plugins/CLient/PluginCommon.h"
#include <stdio.h>

static void* __stdcall ClientInfoDeserializer(char* packet, int size)
{
	static base::ServerClientInfo info;
	if (info.ParseFromArray(packet, size)) return &info;
	return nullptr;
}
static void* __stdcall PluginDeserializer(char* packet, int size)
{
	static base::ServerPlugin plugin;
	if (plugin.ParseFromArray(packet, size)) return &plugin;
	return nullptr;
}
static void* __stdcall SetStateDeserializer(char* packet, int size)
{
	static base::ServerSetState state;
	if (state.ParseFromArray(packet, size)) return &state;
	return nullptr;
}
static void* __stdcall TimerDeserializer(char* packet, int size)
{
	static base::ServerLobbyTimer timer;
	if (timer.ParseFromArray(packet, size)) return &timer;
	return nullptr;
}
static void* __stdcall LobbyVote(char* packet, int size)
{
	static base::ServerLobbyVote vote;
	if (vote.ParseFromArray(packet, size)) return &vote;
	return nullptr;
}

void NetClient::SteamNetClientConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetClient* client = (NetClient*)pInfo->m_info.m_nUserData;
	if (client == (NetClient*)0xFFFFFFFFFFFFFFFF) return;

	switch (pInfo->m_info.m_eState)
	{
	case k_ESteamNetworkingConnectionState_ClosedByPeer:
	case k_ESteamNetworkingConnectionState_ProblemDetectedLocally:
	{
		client->socket.networking->CloseConnection(pInfo->m_hConn, 0, nullptr, false);
		client->socket.socket = 0;
		client->connectionState = State::Disconnected;
		if (client->disconnectCB) client->disconnectCB(client->appData, client->local);
		break;
	}

	case k_ESteamNetworkingConnectionState_Connected:
	case k_ESteamNetworkingConnectionState_Connecting:
		client->connectionState = State::ReceivedAnswer;
		break;

	case k_ESteamNetworkingConnectionState_None:
	default:
		break;
	}


}
NetClient* NetClient::Create()
{
	NetClient* out = new NetClient();
	out->socket.networking = SteamNetworkingSockets();

	out->SetLobbyDeserializers();

	out->SetDeserializer(SetStateDeserializer, Server_SetState);
	out->SetDeserializer(PluginDeserializer, Server_Plugin);
	out->SetDeserializer(ClientInfoDeserializer, Server_ClientInfo);

	out->SetCallback((ClientPacketFunction)ServerInfoCallback, Server_ClientInfo);


	return out;
}
NetResult NetClient::Connect(const char* ip, uint32_t port, const std::string& name)
{
	NetResult joinRes;
	joinRes.success = false;
	if (connectionState != State::Disconnected)
	{
		joinRes.success = true;
		return joinRes;
	}
	NameValidationResult validation = ValidateName(name);
	switch (validation)
	{
	case Name_Ok:
		break;
	case Name_ErrSmall:
		joinRes.reason = "Name to short";
		return joinRes;
	case Name_ErrLarge:
		joinRes.reason = "Name to long";
		return joinRes;
	case Name_ErrSymbol:
		joinRes.reason = "Invalid Symbol in Name";
		return joinRes;
	default:
		break;
	}
	uint32_t ipAddr = ParseIP(ip);
	if (!ipAddr) {
		joinRes.reason = "Failed to Parse IP";
		return joinRes;
	}


	SteamNetworkingIPAddr addr;
	addr.Clear();
	addr.SetIPv4(ipAddr, port);

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetClientConnectionStatusChangedCallback);
	HSteamListenSocket sock = socket.networking->ConnectByIPAddress(addr, 1, &opt);
	if (sock == k_HSteamListenSocket_Invalid)
	{
		joinRes.reason = "Failed to Create Socket";
		return joinRes;
	}
	socket.socket = sock;
	socket.networking->SetConnectionUserData(sock, (int64)this);


	base::ClientJoin join;
	join.set_name(name);
	SendData(Client_Join, &join, SendFlags::Send_Reliable);

	connectionState = State::Connecting;
	joinRes.success = true;
	return joinRes;
}
void NetClient::Disconnect() {
    
    SendData(Client_Disconnect, nullptr, SendFlags::Send_Reliable);
	this->socket.networking->CloseConnection(socket.socket, 1, "bye", false);
    connectionState = State::Disconnected;
}

NetClient::~NetClient()
{
}

bool NetClient::IsConnected() const
{
	if (connectionState == State::Disconnected) return false;

	SteamNetConnectionRealTimeStatus_t stat;
	EResult res = socket.networking->GetConnectionRealTimeStatus(socket.socket, &stat, 0, NULL);
	if (res == EResult::k_EResultOK)
	{
		return (stat.m_eState == ESteamNetworkingConnectionState::k_ESteamNetworkingConnectionState_Connected);
	}
	return false;
}

ClientConnection* NetClient::GetSelf()
{
	return local;
}

ClientConnection* NetClient::GetConnection(uint16_t id)
{
	if (id < MAX_PLAYERS)
	{
		if (socket.connected[id].isConnected != ConnectionState::Disconnected)
		{
			return &socket.connected[id];
		}
	}
	return nullptr;
}

void NetClient::SetCallback(ClientPacketFunction fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).receiver = fn;
}
void NetClient::SetDeserializer(DeserializationFunc fn, uint16_t packetID)
{
	if (callbacks.size() < packetID + 1)
	{
		callbacks.resize(packetID + 1);
	}
	callbacks.at(packetID).deserializer = fn;
}

void NetClient::SetClientInfoCallback(ClientInfoCallbackFunction fn)
{
	infoCB = fn;
}

void NetClient::SetDisconnectCallback(ClientDisconnectCallbackFunction fn)
{
	disconnectCB = fn;
}

bool NetClient::SendDataRaw(uint16_t packetID, const void* data, uint32_t size, uint32_t flags)
{
    if(size >= MAX_MESSAGE_LENGTH) {
        LOG("[WARNING]: MESSAGE IS OVER THE SIZE LIMIT\n");
    }
    *(uint16_t*)tempStorage = packetID;
    if(data) {
	    memcpy(&tempStorage + sizeof(packetID), data, size);
    } else { 
        size = 0;
    }
	int64 outMsgNum = 0;
	EResult res = socket.networking->SendMessageToConnection(socket.socket, tempStorage, size + sizeof(packetID), flags, &outMsgNum);
	return EResult::k_EResultOK == res;
}

bool NetClient::SendData(uint16_t packetID, google::protobuf::Message* msg, uint32_t flags) {
    *(uint16_t*)tempStorage = packetID;
    size_t size = 0;
    if(msg) {
        size = msg->ByteSizeLong();
        if(size < MAX_MESSAGE_LENGTH) {
            if(!msg->SerializeToArray(tempStorage + sizeof(packetID), (int)size)) { 
                return false;
            }
        }
    }
    if(size < MAX_MESSAGE_LENGTH) {
        int64 outMsgNum = 0;
	    EResult res = socket.networking->SendMessageToConnection(socket.socket, tempStorage, (uint32_t)(size + sizeof(packetID)), (int)flags, &outMsgNum);
        return EResult::k_EResultOK == res;
    }
    else {
        LOG("[WARNING]: MESSAGE IS OVER THE SIZE LIMIT\n");
    }
    return false;
}

bool NetClient::IsP2P() const
{
	return false;
}

void NetClient::Poll()
{
	if (socket.socket == k_HSteamListenSocket_Invalid) return;
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = socket.networking->ReceiveMessagesOnConnection(socket.socket, &pIncomingMsg, 1);
		if (numMsgs == 0)
			break;
		if (numMsgs < 0) return;
		const uint32 size = pIncomingMsg->GetSize();
		if (size < sizeof(uint16_t)) {
			pIncomingMsg->Release();
			continue;
		}
		const uint16_t* data = (const uint16_t*)pIncomingMsg->GetData();
		const uint16_t packetID = *data;
		if (packetID < callbacks.size())
		{
			NetClient::CallbackInfo& info = callbacks.at(packetID);
			if (info.receiver)
			{
				const void* packet = data + 1;
				if (info.deserializer) {
					packet = info.deserializer((char*)packet, size - 2);
				}
				if (packet)
				{
					info.receiver(appData, (void*)packet, size - 2);
				}
			}
		}

		pIncomingMsg->Release();
	}
}

void NetClient::SetLobbyDeserializers()
{
	SetDeserializer(TimerDeserializer, Server_LobbyTimer);
	SetDeserializer(LobbyVote, Server_LobbyVote);
}


bool __stdcall NetClient::ServerInfoCallback(ApplicationData* app, base::ServerClientInfo* info, int size)
{
	
	const int32 idVal = info->data().id();
	NetClient* c = (NetClient*)app->net;
	if (idVal < MAX_PLAYERS && idVal >= 0)
	{
		ClientConnection* cl = &c->socket.connected[idVal];
		cl->isAdmin = info->data().is_admin();
		cl->id = idVal;
		cl->isConnected = ConnectionState::Connected;
		cl->name = info->data().name();
		if (info->is_local())
		{
			c->local = cl;
		}
		if (info->is_connected())
		{
			if (c->infoCB)
			{
				c->infoCB(c->appData, cl);
			}
		}
		else
		{
			if(c->disconnectCB)
			{
				c->disconnectCB(c->appData, cl);
			}
		}
	}
	else
	{
		c->socket.networking->CloseConnection(c->socket.socket, 1, "received invalid player id", false);
	}
	return true;
}

