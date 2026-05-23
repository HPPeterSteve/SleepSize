/*
 * sleep_json.c
 * Exporta metadados do diretório congelado para JSON.
 * Sem lib externa — write() manual.
 */

#include "sleepsize.h"

#include <dirent.h>
#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <string.h>
#include <sys/stat.h>
#include <time.h>
#include <unistd.h>

/* Escreve string no fd — helper interno */
static int wstr(int fd, const char *s)
{
    size_t  len = strlen(s);
    ssize_t nw  = write(fd, s, len);
    return (nw == (ssize_t)len) ? 0 : -1;
}

/* Escreve string JSON escapada (só escapa aspas e barras) */
static int wjstr(int fd, const char *s)
{
    char buf[4];
    wstr(fd, "\"");
    for (; *s; s++) {
        if (*s == '"' || *s == '\\') {
            buf[0] = '\\'; buf[1] = *s; buf[2] = '\0';
            wstr(fd, buf);
        } else {
            buf[0] = *s; buf[1] = '\0';
            wstr(fd, buf);
        }
    }
    return wstr(fd, "\"");
}

/*
 * sleep_export_json
 * Percorre o diretório e exporta nome, tamanho e data de modificação
 * de cada arquivo regular para um JSON em out_path.
 *
 * Formato:
 * {
 *   "path": "/caminho/do/dir",
 *   "files": [
 *     {"name": "arquivo.txt", "size": 1024, "mtime": "2025-05-01 14:30:00"},
 *     ...
 *   ]
 * }
 */
SSError sleep_export_json(const SleepDir *dir, const char *out_path)
{
    if (!dir || !out_path) return SS_ERR;

    DIR *d = opendir(dir->path);
    if (!d) {
        fprintf(stderr, "[sleepsize] opendir(%s) falhou: %m\n", dir->path);
        return SS_ERR;
    }

    int fd = open(out_path,
                  O_WRONLY | O_CREAT | O_EXCL | O_NOFOLLOW,
                  0644);
    if (fd < 0) {
        fprintf(stderr, "[sleepsize] open(json=%s) falhou: %m\n", out_path);
        closedir(d);
        return SS_ERR;
    }

    /* Abre JSON */
    wstr(fd, "{\n");
    wstr(fd, "  \"path\": ");
    wjstr(fd, dir->path);
    wstr(fd, ",\n  \"files\": [\n");

    struct dirent *ent;
    char           entry_path[PATH_MAX];
    struct stat    est;
    char           timebuf[32];
    bool           first = true;

    while ((ent = readdir(d)) != NULL) {
        if (ent->d_name[0] == '.') continue; /* ignora . e .. e ocultos */

        int r = snprintf(entry_path, sizeof(entry_path),
                         "%s/%s", dir->path, ent->d_name);
        if (r < 0 || (size_t)r >= sizeof(entry_path)) continue;

        if (stat(entry_path, &est) != 0 || !S_ISREG(est.st_mode)) continue;

        /* Formata data de modificação */
        struct tm *tm_info = localtime(&est.st_mtime);
        if (!tm_info) continue;
        strftime(timebuf, sizeof(timebuf), "%Y-%m-%d %H:%M:%S", tm_info);

        /* Separador entre entradas */
        if (!first) wstr(fd, ",\n");
        first = false;

        wstr(fd, "    {\"name\": ");
        wjstr(fd, ent->d_name);

        char numbuf[32];
        snprintf(numbuf, sizeof(numbuf), "%lld", (long long)est.st_size);
        wstr(fd, ", \"size\": ");
        wstr(fd, numbuf);

        wstr(fd, ", \"mtime\": ");
        wjstr(fd, timebuf);
        wstr(fd, "}");
    }

    /* Fecha JSON */
    wstr(fd, "\n  ]\n}\n");

    fsync(fd);
    close(fd);
    closedir(d);

    fprintf(stderr, "[sleepsize] metadados exportados: %s\n", out_path);
    return SS_OK;
}
