#include "thread_pool.h"

// 生成sha256码
void sha256(const char *str, char outputBuffer[65])
{
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        return;
    }

    const EVP_MD *md = EVP_sha256();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestUpdate(ctx, str, strlen(str)) != 1) {
        fprintf(stderr, "EVP_DigestUpdate failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestFinal_ex(ctx, hash, &lengthOfHash) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // Convert the hash to a hexadecimal string
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[64] = '\0'; // Null-terminate the string
}

// 生成sha512码
void sha512(const char *str, char outputBuffer[129])
 {
    EVP_MD_CTX *ctx = EVP_MD_CTX_new();
    if (ctx == NULL) {
        fprintf(stderr, "EVP_MD_CTX_new failed\n");
        return;
    }

    const EVP_MD *md = EVP_sha512();
    unsigned char hash[EVP_MAX_MD_SIZE];
    unsigned int lengthOfHash = 0;

    if (EVP_DigestInit_ex(ctx, md, NULL) != 1) {
        fprintf(stderr, "EVP_DigestInit_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestUpdate(ctx, str, strlen(str)) != 1) {
        fprintf(stderr, "EVP_DigestUpdate failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    if (EVP_DigestFinal_ex(ctx, hash, &lengthOfHash) != 1) {
        fprintf(stderr, "EVP_DigestFinal_ex failed\n");
        EVP_MD_CTX_free(ctx);
        return;
    }

    EVP_MD_CTX_free(ctx);

    // Convert the hash to a hexadecimal string
    for (unsigned int i = 0; i < lengthOfHash; i++) {
        sprintf(outputBuffer + (i * 2), "%02x", hash[i]);
    }
    outputBuffer[128] = '\0'; // Null-terminate the string
}