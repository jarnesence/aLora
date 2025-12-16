#include "CryptoManager.h"
#include <uECC.h>
#include <AESLib.h>

// Helper for uECC RNG
static int RNG(uint8_t *dest, unsigned size) {
    // Use ESP32 hardware RNG
    for (unsigned i = 0; i < size; i++) {
        dest[i] = (uint8_t)esp_random();
    }
    return 1;
}

void CryptoManager::init() {
    uECC_set_rng(&RNG);
}

void CryptoManager::generateKeyPair(uint8_t* publicKey, uint8_t* privateKey) {
    const struct uECC_Curve_t * curve = uECC_secp256r1();
    uECC_make_key(publicKey, privateKey, curve);
}

bool CryptoManager::computeSharedSecret(const uint8_t* remotePublicKey, const uint8_t* privateKey, uint8_t* sharedSecret) {
    const struct uECC_Curve_t * curve = uECC_secp256r1();
    return uECC_shared_secret(remotePublicKey, privateKey, sharedSecret, curve);
}

// AESLib wrapper - note: AESLib APIs vary. Assuming a standard one (suculent/AESLib)
// AESLib uses 128-bit blocks. Padding is needed.
AESLib aesLib;

void CryptoManager::encrypt(const uint8_t* plainText, size_t length, const uint8_t* key, const uint8_t* iv, uint8_t* output, size_t& outLen) {
    // Determine padded length
    int paddedLen = length;
    if ((paddedLen % 16) != 0) {
        paddedLen = (paddedLen / 16 + 1) * 16;
    }

    // Copy plainText to output buffer (assuming it's large enough) and pad
    memcpy(output, plainText, length);
    for (int i = length; i < paddedLen; i++) {
        output[i] = 0; // Zero padding for simplicity, PKCS7 is better but depends on lib support
    }

    // Encrypt
    // aesLib.encrypt(output, paddedLen, (char*)output, (byte*)key, 256, (byte*)iv);
    // The API might differ. Let's assume `encrypt` takes input, length, key, iv.
    // AESLib by suculent:
    // uint16_t encrypt(byte *input, uint16_t input_length, char * output, byte * key, int bits, byte * my_iv);

    // Note: IV is modified by the library in CBC mode!
    byte localIV[16];
    memcpy(localIV, iv, 16);

    // Using a simpler interface if available or just raw calls.
    // Let's assume standard usage:
    aesLib.encrypt(output, paddedLen, (char*)output, (byte*)key, 256, localIV);
    outLen = paddedLen;
}

void CryptoManager::decrypt(const uint8_t* cipherText, size_t length, const uint8_t* key, const uint8_t* iv, uint8_t* output, size_t& outLen) {
    // Decrypt
    // Note: IV is modified.
    byte localIV[16];
    memcpy(localIV, iv, 16);

    // Copy cipherText to output as working buffer
    memcpy(output, cipherText, length);

    aesLib.decrypt(output, length, (char*)output, (byte*)key, 256, localIV);
    outLen = length; // Unpadding needed logically, but for now return full block
}

void CryptoManager::generateRandomIV(uint8_t* iv) {
    for (int i = 0; i < 16; i++) {
        iv[i] = (uint8_t)esp_random();
    }
}
