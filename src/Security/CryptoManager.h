#ifndef CRYPTO_MANAGER_H
#define CRYPTO_MANAGER_H

#include <Arduino.h>

class CryptoManager {
public:
    static void init();

    // Key Management
    static void generateKeyPair(uint8_t* publicKey, uint8_t* privateKey);
    static bool computeSharedSecret(const uint8_t* remotePublicKey, const uint8_t* privateKey, uint8_t* sharedSecret);

    // Encryption/Decryption (AES-256-CBC)
    static void encrypt(const uint8_t* plainText, size_t length, const uint8_t* key, const uint8_t* iv, uint8_t* output, size_t& outLen);
    static void decrypt(const uint8_t* cipherText, size_t length, const uint8_t* key, const uint8_t* iv, uint8_t* output, size_t& outLen);

    // Helpers
    static void generateRandomIV(uint8_t* iv);
};

#endif // CRYPTO_MANAGER_H
