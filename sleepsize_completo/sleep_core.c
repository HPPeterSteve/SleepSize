/*
 * sleep_core.c
 * Lógica principal: inicialização, freeze, unfreeze e adição de arquivo.
 */

#include "sleepsize.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <unistd.h>

/* ─── Helpers internos ───────────────────────────────────────────────────── */

/*
 * chmod_recursive
 * Aplica `mode` recursivamente em todos os arquivos e diretórios dentro de path.
 * dir_mode = modo para diretórios, file_mode = modo para arquivos.
 */
static int chmod_recursive(const char *path, mode_t dir_mode, mode_t file_mode)
{
    struct stat st;
    if (lstat(path, &st) != 0) return -1;

    if (S_ISDIR(st.st_mode)) {
        DIR *dir = opendir(path);
        if (!dir) return -1;

        struct dirent *ent;
        char child[PATH_MAX];
        while ((ent = readdir(dir)) != NULL) {
            if (strcmp(ent->d_name, ".") == 0 ||
                strcmp(ent->d_name, "..") == 0) continue;

            int r = snprintf(child, sizeof(child), "%s/%s", path, ent->d_name);
            if (r < 0 || (size_t)r >= sizeof(child)) continue;
            chmod_recursive(child, dir_mode, file_mode);
        }
        closedir(dir);
        chmod(path, dir_mode);
    } else if (S_ISREG(st.st_mode)) {
        chmod(path, file_mode);
    }

    return 0;
}

/* ─── API pública ────────────────────────────────────────────────────────── */

/*
 * sleep_init
 * Inicializa SleepDir com o path e senha opcional.
 * Não congela ainda — apenas prepara o estado.
 */
SSError sleep_init(SleepDir *dir, const char *path, const char *passwd)
{
    if (!dir || !path) return SS_ERR;
    memset(dir, 0, sizeof(*dir));
    dir->fan_fd = -1;

    /* Valida e canonicaliza o path */
    if (!path_is_safe(path, dir->path)) {
        fprintf(stderr, "[sleepsize] path inválido: %s\n", path);
        return SS_ERR_PATH;
    }

    /* Verifica que é um diretório */
    struct stat st;
    if (stat(dir->path, &st) != 0 || !S_ISDIR(st.st_mode)) {
        fprintf(stderr, "[sleepsize] não é um diretório: %s\n", dir->path);
        return SS_ERR_NOTFOUND;
    }

    /* Configura senha se fornecida */
    if (passwd && passwd[0] != '\0') {
        sha256_hex((const uint8_t *)passwd, strlen(passwd), dir->passwd_hash);
        dir->has_password = true;
    }

    /* Inicia fanotify */
    dir->fan_fd = fan_init();
    if (dir->fan_fd >= 0)
        fan_mark_dir(dir->fan_fd, dir->path);

    return SS_OK;
}

/*
 * sleep_freeze
 * Congela o diretório:
 *   1. chmod 0555 nos subdiretórios (executável mas não escrevível)
 *   2. chmod 0444 em todos os arquivos (somente leitura)
 *   3. Marca como frozen
 */
SSError sleep_freeze(SleepDir *dir)
{
    if (!dir || dir->path[0] == '\0') return SS_ERR;
    if (dir->frozen) return SS_OK; /* já congelado */

    if (chmod_recursive(dir->path, 0555, 0444) != 0) {
        fprintf(stderr, "[sleepsize] chmod falhou em %s: %m\n", dir->path);
        return SS_ERR;
    }

    dir->frozen = true;
    fprintf(stderr, "[sleepsize] freeze aplicado: %s\n", dir->path);
    return SS_OK;
}

/*
 * sleep_unfreeze
 * Remove o freeze do diretório.
 * Se has_password, exige senha correta.
 */
SSError sleep_unfreeze(SleepDir *dir, const char *passwd)
{
    if (!dir || dir->path[0] == '\0') return SS_ERR;
    if (!dir->frozen) return SS_OK;

    /* Verifica senha se necessário */
    if (dir->has_password) {
        if (!passwd || passwd[0] == '\0') return SS_ERR_PASSWD;
        char hash[65];
        sha256_hex((const uint8_t *)passwd, strlen(passwd), hash);
        if (strcmp(hash, dir->passwd_hash) != 0) {
            fprintf(stderr, "[sleepsize] senha incorreta\n");
            return SS_ERR_PASSWD;
        }
    }

    /* Restaura permissões */
    if (chmod_recursive(dir->path, 0755, 0644) != 0) {
        fprintf(stderr, "[sleepsize] chmod (unfreeze) falhou: %m\n");
        return SS_ERR;
    }

    dir->frozen = false;
    fprintf(stderr, "[sleepsize] freeze removido: %s\n", dir->path);
    return SS_OK;
}

/*
 * sleep_add_file
 * Adiciona um arquivo ao diretório congelado.
 * Fluxo: verifica senha → libera escrita temporária → copia → recongela.
 */
SSError sleep_add_file(SleepDir *dir, const char *file_path, const char *passwd)
{
    if (!dir || !file_path) return SS_ERR;

    /* Verifica senha */
    if (dir->has_password) {
        if (!passwd || passwd[0] == '\0') return SS_ERR_PASSWD;
        char hash[65];
        sha256_hex((const uint8_t *)passwd, strlen(passwd), hash);
        if (strcmp(hash, dir->passwd_hash) != 0) {
            fprintf(stderr, "[sleepsize] senha incorreta para adicionar arquivo\n");
            return SS_ERR_PASSWD;
        }
    }

    /* Valida o arquivo de origem */
    char safe_src[PATH_MAX];
    if (!path_is_safe(file_path, safe_src)) {
        fprintf(stderr, "[sleepsize] path de origem inválido: %s\n", file_path);
        return SS_ERR_PATH;
    }

    struct stat src_st;
    if (stat(safe_src, &src_st) != 0 || !S_ISREG(src_st.st_mode)) {
        fprintf(stderr, "[sleepsize] arquivo de origem não encontrado: %s\n", safe_src);
        return SS_ERR_NOTFOUND;
    }

    /* Extrai nome do arquivo */
    const char *fname = strrchr(safe_src, '/');
    fname = fname ? fname + 1 : safe_src;

    /* Constrói destino */
    char dst[PATH_MAX];
    int r = snprintf(dst, sizeof(dst), "%s/%s", dir->path, fname);
    if (r < 0 || (size_t)r >= sizeof(dst)) return SS_ERR_PATH;

    /* Abre janela de escrita temporária */
    chmod(dir->path, 0755);

    SSError rc = SS_OK;
    if (copy_file(safe_src, dst) != 0) {
        fprintf(stderr, "[sleepsize] cópia falhou: %s → %s: %m\n", safe_src, dst);
        rc = SS_ERR;
    } else {
        fprintf(stderr, "[sleepsize] arquivo adicionado: %s\n", fname);
    }

    /* Recongela imediatamente — mesmo em caso de erro */
    chmod(dir->path, 0555);

    return rc;
}

/*
 * sleep_rollback
 * Desfaz alterações (unfreeze) sem commit.
 */
SSError sleep_rollback(SleepDir *dir, const char *passwd)
{
    if (!dir || dir->path[0] == '\0') return SS_ERR;
    return sleep_unfreeze(dir, passwd);
}

/*
 * sleep_commit
 * Confirma o estado congelado (no-op, pois o freeze já está aplicado).
 */
SSError sleep_commit(SleepDir *dir, const char *passwd)
{
    if (!dir || dir->path[0] == '\0') return SS_ERR;
    if (dir->has_password && (!passwd || passwd[0] == '\0')) return SS_ERR_PASSWD;
    if (dir->has_password) {
        char hash[65];
        sha256_hex((const uint8_t *)passwd, strlen(passwd), hash);
        if (strcmp(hash, dir->passwd_hash) != 0) return SS_ERR_PASSWD;
    }
    fprintf(stderr, "[sleepsize] estado congelado confirmado: %s\n", dir->path);
    return SS_OK;
}

/*
 * sleep_destroy
 * Libera recursos do SleepDir (fecha fanotify, zera estrutura).
 */
void sleep_destroy(SleepDir *dir)
{
    if (!dir) return;
    fan_close(dir->fan_fd);
    memset(dir, 0, sizeof(*dir));
    dir->fan_fd = -1;
}
