#include "NetServer.h"
#include "NetCommon.h"


void NetServer::SteamNetServerConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetServer* server = (NetServer*)pInfo->m_info.m_nUserData;
	ServerConnection* conn = server->GetNew();
	if (conn)
	{
		conn->conn = pInfo->m_hConn;
		conn->isConnected = ConnectionState::Connecting;
		conn->id = server->GetClientID(conn);
		conn->state = ClientState::INVALID;
		conn->name = "";
		conn->activePlugin = INVALID_ID;
	}
	else
	{
		server->socket.networking->CloseConnection(pInfo->m_hConn, 1, "Server is Full", false);
	}
}

NetServer* NetServer::Create(const char* ip, uint32_t port)
{
	uint32_t ipAddr = 0;
	{
		int ipLen = strnlen(ip, 100);
		int curIdx = 0;
		for (int i = 0; i < ipLen; i++)
		{
			if (curIdx > 4) return nullptr;

			if (i == 0)
			{
				const int val = atoi(ip + i);
				ipAddr |= val << (8 * curIdx);
				curIdx++;
			}
			else if (ip[i] == '.')
			{
				const int val = atoi(ip + i + 1);
				ipAddr |= val << (8 * curIdx);
				curIdx++;
			}
		}
	}


	NetServer* out = new NetServer();
	out->socket.networking = SteamNetworkingSockets();

	SteamNetworkingIPAddr serverLocalAddr;
	serverLocalAddr.Clear();
	serverLocalAddr.SetIPv4(ipAddr, port);

	SteamNetworkingConfigValue_t opt;
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetServerConnectionStatusChangedCallback);
	out->socket.socket = out->socket.networking->CreateListenSocketIP(serverLocalAddr, 1, &opt);
	if (out->socket.socket == k_HSteamListenSocket_Invalid) return nullptr;
	out->group = out->socket.networking->CreatePollGroup();
	if (out->group == k_HSteamNetPollGroup_Invalid) return nullptr;
	out->socket.networking->SetConnectionUserData(out->socket.socket, (int64)out);

	return out;
}

NetServer::~NetServer()
{

}

ServerConnection* NetServer::GetConnection(uint16_t id)
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
	while (true)
	{
		ISteamNetworkingMessage* pIncomingMsg = nullptr;
		int numMsgs = socket.networking->ReceiveMessagesOnPollGroup(group, &pIncomingMsg, 1);

		if (numMsgs == 0)
			break;
		if (numMsgs < 0) return;

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
uint16_t NetServer::GetClientID(ServerConnection* conn)
{
	uintptr_t id = ((uintptr_t)conn - (uintptr_t)socket.connected) / sizeof(ServerConnection);
	if (id < 0xFFFF)
	{
		return id;
	}
	return INVALID_ID;
}