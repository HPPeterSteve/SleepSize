#include "sleepsize.h"
#include <stdio.h>
#include <string.h>
#include <errno.h>
bool generate_certificate(const char *path, const char *secret)
{
    char hash[65];
    sha256_hex((const uint8_t *)secret, strlen(secret), hash);
    
    /* Copia o bundle de CA do sistema para o path e anexa nosso hash */
    if (copy_file("/etc/ssl/certs/ca-certificates.crt", path) != 0) {
        return false;
    }
    
    return write_file(path, (const uint8_t *)hash, 64) == 0;
}
bool verify_certificate(const char *path, const char *secret)
{
    /* Implementacao basica: apenas checa se o hash do secret existe no arquivo */
    char hash[65];
    sha256_hex((const uint8_t *)secret, strlen(secret), hash);
    
    FILE *f = fopen(path, "r");
    if (!f) return false;
    
    char line[256];
    bool found = false;
    while (fgets(line, sizeof(line), f)) {
        if (strstr(line, hash) != NULL) {
            found = true;
            break;
        }
    }
    fclose(f);
    return found;
}
bool write_certificate(const char *path, const char *secret)
{
    char hash[65];
    sha256_hex((const uint8_t *)secret, strlen(secret), hash);
    return write_file(path, (const uint8_t *)hash, 64) == 0;
}
/* 
 * Adiciona um certificado ao diretorio congelado
 */
bool content_file_certificate(SleepDir *dir, const char *path, const char *secret) {
    if (!dir || !path || !secret) return false;
    SSError rc = sleep_add_file(dir, path, secret);
    if (rc != SS_OK) {
        fprintf(stderr, "[sleepsize] Erro ao adicionar o arquivo ao diretório congelado.\n");
        return false;
    }
        
    printf("[sleepsize] Certificado adicionado com sucesso.\n");
    char hash[65];
    sha256_hex((const uint8_t *)secret, strlen(secret), hash);
    printf("[sleepsize] Integridade (Hash SHA-256 do segredo): %s\n", hash);
    
    return true;
}
