#pragma once
#include <stdint.h>
#include <thread>
#include <vector>
#include <mutex>
#include "Cryptography.h"

// ADMIN/CREATOR OF THE ROOM
#define ADMIN_GROUP_MASK (1 << 31)
#define STANDARD_GROUP_MASK 1

enum class NetError
{
	OK,
	E_INIT,			// failed to initialize
	E_RESOLVE,		// failed to Resolve the address
	E_CONNECT,		// failed to Connect
	E_LISTEN,		// failed to Listen
	E_READ,			// failed to Read bytes from socket
	E_HANDSHAKE,	// failed to perform handshake
	E_VERSION,		// failed Version mismatch
};

enum class PacketID
{
	HANDSHAKE = 0,
	CHECK = 1,
	JOIN = 2,
	CREATE = 3,
	SYNC_REQUEST = 4,	// these need to be seperate as the server can ask the Admin for the current SYNC data
	SYNC_RESPONSE = 5,	// these need to be seperate as the server can ask the Admin for the current SYNC data
	ADD_CLIENT = 6,
	REMOVE_CLIENT = 7,
	// END OF GENERAL MESSAGES


	VOTE = 8,
	VOTE_SYNC = 9,
};

struct PacketHeader
{
	PacketHeader() { type = 0; size = 0; group = 0x0; additionalData = 0; }
	constexpr PacketHeader(uint32_t t, uint32_t sz) : type(t), size(sz), group(0x0), additionalData(0) { }
	constexpr PacketHeader(PacketID t, uint32_t sz) : type((uint32_t)t), size(sz), group(0x0), additionalData(0) { }
	constexpr PacketHeader(PacketID t, uint32_t gr, uint32_t sz) : type((uint32_t)t), size(sz), group(gr), additionalData(0) { }
	constexpr PacketHeader(PacketID t, uint32_t gr, uint32_t dat, uint32_t sz) : type((uint32_t)t), size(sz), group(gr), additionalData(dat) { }
	uint32_t type;
	uint32_t size;
	uint32_t group;
	uint32_t additionalData;
};
struct Packet
{
	PacketHeader header;
	std::vector<char> body;
};

struct ValidationPacket
{
	uint8_t PacketData[64];
};

// if this is changed, KEY_LEN needs to be changed as well !
#define KEY_BITS_512
#ifdef KEY_BITS_128
typedef uint128_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU128;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU128;
#elif defined(KEY_BITS_256)
typedef uint256_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU256; 
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU256;
#elif defined(KEY_BITS_512)
typedef uint512_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU512;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU512;
#elif defined(KEY_BITS_1024)
typedef uint1024_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU1024;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU1024;
#elif defined(KEY_BITS_2048)
typedef uint2048_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU2048;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU2048;
#elif defined(KEY_BITS_4096)
typedef uint4096_t ukey_t;
typedef void(*KeyGenFunc)(ukey_t* publicKey, ukey_t* privateKey);
typedef void(*KeyGenSharedFunc)(ukey_t* shared, const ukey_t publicKey, const ukey_t privateKey);
static constexpr KeyGenFunc GenKeyPair = GenKeyPairU4096;
static constexpr KeyGenSharedFunc GenSharedSecret = GenSharedSecretU4096;
#endif




struct TCPSocket;
struct TCPServerSocket;
typedef void(*ConnectionFunc)(void* obj, struct Connection* conn);
typedef void(*ClientPacketCallback)(void* userData, Packet* pack);
typedef void(*ServerPacketCallback)(void* userData, Connection* conn, Packet* pack);
typedef void(*ConnectCallback)(void* userData, TCPServerSocket* sock, Connection* conn);
typedef void(*DisconnectCallback)(void* userData, TCPServerSocket* sock, Connection* removed);
typedef void(*ClientDisconnectCallback)(void* userData, TCPSocket* sock, Connection* removed);

struct Connection
{
	Connection();
	Connection(uintptr_t sock);
	Connection(uintptr_t sock, ConnectionFunc socketCallback);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj);
	Connection(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);
	~Connection();

	void Init(uintptr_t sock, ConnectionFunc socketCallback, void* obj, const ukey_t& key);

	void SendData(PacketID id, size_t size, const void* data);
	void SendData(PacketID id, uint32_t group, size_t size, const void* data);
	void SendData(PacketID id, uint32_t group, uint32_t additionalData, size_t size, const void* data);
	void SendData(PacketID id, const std::string& str);
	void SendData(PacketID id, uint32_t group, const std::string& str);
	void SendData(PacketID id, uint32_t group, uint32_t additionalData, const std::string& str);

	void SetCryptoKey(const ukey_t& k);

	CryptoContext ctx;
	uintptr_t socket;
	std::thread listener;
	Packet storedPacket;
};

class TCPSocket
{
public:
	TCPSocket();
	~TCPSocket() { running = false; }
	NetError Connect(const char* host, const char* port);
	void Disconnect();

	void SetPacketCallback(ClientPacketCallback cb, void* userData = nullptr);
	void SetDisconnectCallback(ClientDisconnectCallback cb, void* userData = nullptr);

	void SendData(PacketID id, uint32_t size, const uint8_t* data);
	void SendData(PacketID id, const std::string& data);
	void SendData(PacketID id, uint32_t group, uint32_t additionalData, const std::string& data);

private:
	void SendDataUnencrypted(PacketID id, uint32_t size, const uint8_t* data);
	static void SocketListenFunc(void* obj, struct Connection* conn);
	uintptr_t sock;
	ClientPacketCallback packCb = nullptr;
	void* userDataPacketCB = nullptr;
	ClientDisconnectCallback disconnectCB = nullptr;
	void* userDataDisconnectCB = nullptr;
	Connection serverConn;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running = true;
};

class TCPServerSocket
{
public:
	TCPServerSocket();
	NetError Create(const char* host, const char* port);
	NetError AcceptConnection();

	void SetConnectCallback(ConnectCallback cb, void* userData = nullptr);
	void SetPacketCallback(ServerPacketCallback cb, void* userData = nullptr);
	void SetDisconnectCallback(DisconnectCallback cb, void* userData = nullptr);

	void RemoveAll();

	void RemoveClient(Connection* conn);

	static void TCPServerListenToClient(void* server, Connection* conn);
private:


	void AddClient(uintptr_t connSocket, const ukey_t& key);
	void RemoveClient(uintptr_t connSocket);
	void RemoveClientAtIdx(size_t idx);


	ServerPacketCallback packetCallback;
	void* userDataPacketCB;
	ConnectCallback connectCallback;
	void* userDataConnectCB;
	DisconnectCallback disconnectCallback;
	void* userDataDisconnectCB;
	uintptr_t sock;
	std::mutex clientMut;
	std::vector<Connection*> clients;
	ukey_t privateKey;
	ukey_t publicKey;
	bool running;
};

std::string GetIPAddress(uintptr_t socket);
