#pragma once
#include "NetCommon.h"



struct NetClient : public NetClientInterface
{
	enum State
	{
		Disconnected,
		Connecting,
		ReceivedAnswer,
		Connected,
	};
	static NetClient* Create();

	NetResult Connect(const char* ip, uint32_t port, const std::string& name);


	virtual ~NetClient();

	virtual bool IsConnected() const;

	virtual ClientConnection* GetSelf();
	virtual ClientConnection* GetConnection(uint16_t id);
	virtual void SetCallback(ClientPacketFunction fn, uint16_t packetID);
	virtual void SetDeserializer(DeserializationFunc fn, uint16_t packetID);

	virtual void SetClientInfoCallback(ClientInfoCallbackFunction fn);
	virtual void SetDisconnectCallback(ClientDisconnectCallbackFunction fn);

	virtual bool SendData(uint16_t packetID, const void* data, uint32_t size, uint32_t flags);

	virtual bool IsP2P() const;


	void Poll();

	void SetLobbyDeserializers();


	NetSocketClient socket;
	ApplicationData* appData = nullptr;
	State connectionState = State::Disconnected;
private:

	static void SteamNetClientConnectionStatusChangedCallback(SteamNetConnectionStatusChangedCallback_t* pInfo);

	static bool __stdcall ServerInfoCallback(ApplicationData* app, base::ServerClientInfo* info, int size);

	ClientConnection* local = nullptr;
	ClientDisconnectCallbackFunction disconnectCB = nullptr;
	ClientInfoCallbackFunction infoCB = nullptr;
	struct CallbackInfo
	{
		DeserializationFunc deserializer = nullptr;
		ClientPacketFunction receiver = nullptr;
	};
	std::vector<CallbackInfo> callbacks;
	std::vector<char> tempStorage;
	NetClient() {};

};