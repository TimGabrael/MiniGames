#pragma once
#include <stdint.h>
#include <stddef.h>

#define KEY_LEN 32
#define KEY_EXP_LEN 176
#define BLOCK_LEN 32

class CryptoContext
{
public:
	CryptoContext() {};
	void InitContext(const uint8_t key[KEY_LEN]);
	void EncryptBuffer(uint8_t* buf, size_t length);	// length must be multiple of KEY_LEN !!!
	void DecryptBuffer(uint8_t* buf, size_t length);	// length must be multiple of KEY_LEN !!!
private:
	uint8_t key[KEY_EXP_LEN];
};



union uint128_t {
	struct {
		uint64_t low;
		uint64_t high;
	};
	uint8_t byte[16];
};
union uint256_t {
	struct {
		uint128_t low;
		uint128_t high;
	};
	uint8_t byte[32];
};

void GenKeyPairU128(uint128_t* publicKey, uint128_t* privateKey);
void GenSharedSecretU128(uint128_t* sharedSecret, const uint128_t privateKey, const uint128_t publicKey);
void GenKeyPairU256(uint256_t* publicKey, uint256_t* privateKey);
void GenSharedSecretU256(uint256_t* sharedSecret, const uint256_t privateKey, const uint256_t publicKey);
