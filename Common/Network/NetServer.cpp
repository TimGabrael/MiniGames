#include "NetServer.h"
#include "NetCommon.h"


static void SteamNetConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo)
{
	NetServer* server = (NetServer*)pInfo->m_info.m_nUserData;

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
	opt.SetPtr(k_ESteamNetworkingConfig_Callback_ConnectionStatusChanged, (void*)SteamNetConnectionStatusChangedCallback);
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
	return nullptr;
}

void NetServer::SetCallback(ServerPacketFunction fn, uint16_t packetID)
{
}

void NetServer::SetJoinCallback(ServerJoinCallbackFunction fn)
{
}

void NetServer::SetDisconnectcallback(ServerDisconnectCallbackFunction fn)
{
}

bool NetServer::SendData(ServerConnection* conn, const void* data, uint32_t size, uint32_t flags)
{
	return false;
}

bool NetServer::SendData(uint16_t id, const void* data, uint32_t size, uint32_t flags)
{
	return false;
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

void NetServer::RunCallbacks()
{
	SteamNetworkingSockets()->RunCallbacks();
}