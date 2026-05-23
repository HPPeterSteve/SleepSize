/*
 * sleepsize.h
 * Estruturas, constantes e protótipos do SleepSize.
 * Zero dependência externa — só headers POSIX.
 */

#ifndef SLEEPSIZE_H
#define SLEEPSIZE_H

#include <limits.h>
#include <stdbool.h>
#include <stdint.h>
#include <sys/types.h>
#include <time.h>

/* ─── Versão ─────────────────────────────────────────────────────────────── */
#define SS_VERSION      "0.1.0"
#define SS_MAX_FILES    4096
#define SS_PASSWD_LEN   64      /* SHA-256 hex = 64 chars + \0              */

/* ─── Códigos de retorno ─────────────────────────────────────────────────── */
typedef enum {
    SS_OK           =  0,
    SS_ERR          = -1,
    SS_ERR_PATH     = -2,   /* path inválido ou fora do permitido           */
    SS_ERR_PASSWD   = -3,   /* senha incorreta                              */
    SS_ERR_FROZEN   = -4,   /* operação inválida com diretório congelado    */
    SS_ERR_NOTFOUND = -5,   /* diretório não existe                         */
} SSError;

/* ─── Estado do diretório congelado ──────────────────────────────────────── */
typedef struct {
    char  path[PATH_MAX];       /* caminho canonicalizado do diretório       */
    int   fan_fd;               /* fd do fanotify (-1 se não iniciado)       */
    bool  frozen;               /* true = congelado                          */
    char  passwd_hash[65];      /* SHA-256 hex da senha (vazio = sem senha)  */
    bool  has_password;         /* proteção por senha ativa?                 */
} SleepDir;

/* ─── Entrada de metadados para JSON ─────────────────────────────────────── */
typedef struct {
    char   name[NAME_MAX];
    off_t  size;
    time_t mtime;
} FileEntry;

/* ─── sleep_core.c ───────────────────────────────────────────────────────── */
SSError sleep_init   (SleepDir *dir, const char *path, const char *passwd);
SSError sleep_freeze (SleepDir *dir);
SSError sleep_unfreeze(SleepDir *dir, const char *passwd);
SSError sleep_add_file(SleepDir *dir, const char *file_path, const char *passwd);
void    sleep_destroy(SleepDir *dir);

/* ─── sleep_fan.c ────────────────────────────────────────────────────────── */
int  fan_init      (void);
int  fan_mark_dir  (int fan_fd, const char *path);
void fan_close     (int fan_fd);

/* ─── sleep_json.c ───────────────────────────────────────────────────────── */
SSError sleep_export_json(const SleepDir *dir, const char *out_path);

/* ─── sleep_util.c ───────────────────────────────────────────────────────── */
void sha256_hex  (const uint8_t *data, size_t len, char out[65]);
bool path_is_safe(const char *path, char out_real[PATH_MAX]);
int  copy_file   (const char *src, const char *dst);

#endif /* SLEEPSIZE_H */
