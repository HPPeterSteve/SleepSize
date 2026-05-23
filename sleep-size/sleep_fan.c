/*
 * sleep_fan.c
 * Inicialização e marcação do fanotify.
 * Requer CAP_SYS_ADMIN (ou root) para FAN_CLASS_NOTIF em diretórios.
 */

#include "sleepsize.h"

#include <errno.h>
#include <fcntl.h>
#include <stdio.h>
#include <sys/fanotify.h>
#include <unistd.h>

/*
 * fan_init
 * Inicializa o fanotify para monitoramento de eventos de modificação.
 * Retorna o fd em sucesso ou -1 em falha.
 */
int fan_init(void)
{
    int fd = fanotify_init(FAN_CLASS_NOTIF, O_RDONLY);
    if (fd < 0) {
        fprintf(stderr, "[sleepsize] fanotify_init falhou: %m\n");
        return -1;
    }
    return fd;
}

/*
 * fan_mark_dir
 * Registra o diretório para monitoramento de FAN_MODIFY e FAN_CREATE.
 * Retorna 0 em sucesso, -1 em falha.
 */
int fan_mark_dir(int fan_fd, const char *path)
{
    if (fan_fd < 0 || !path) return -1;

    int rc = fanotify_mark(fan_fd,
                           FAN_MARK_ADD | FAN_MARK_ONLYDIR,
                           FAN_MODIFY | FAN_CREATE | FAN_DELETE,
                           AT_FDCWD,
                           path);
    if (rc < 0) {
        fprintf(stderr, "[sleepsize] fanotify_mark falhou para %s: %m\n", path);
        return -1;
    }
    return 0;
}

/*
 * fan_close
 * Fecha o fd do fanotify.
 */
void fan_close(int fan_fd)
{
    if (fan_fd >= 0)
        close(fan_fd);
}
