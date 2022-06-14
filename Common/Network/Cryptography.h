#pragma once
#include <stdint.h>

#define KEY_LEN 16
#define KEY_EXP_LEN 176
#define BLOCK_LEN 16

class CryptoContext
{
public:
	CryptoContext() {};
	void InitContext(const uint8_t key[KEY_LEN]);
	void EncryptBuffer(uint8_t* buf, size_t length);
	void DecryptBuffer(uint8_t* buf, size_t length);
private:
	uint8_t key[KEY_EXP_LEN];
};
