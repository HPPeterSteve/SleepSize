/*
 * sleep_seccomp.c
 * Filtros Seccomp-BPF compatível com GTK3 e sistema completo.
 * Permite operações de UI e filesystem enquanto bloqueia chamadas perigosas.
 */
#include "sleepsize.h"
#include <stdio.h>
#include <seccomp.h>
#include <errno.h>

/*
 * sleep_apply_seccomp
 * Aplica um filtro permissivo com whitelist de syscalls perigosas.
 * Permite quase tudo exceto ptrace, chroot, e acesso a recursos críticos.
 */
int sleep_apply_seccomp(void) {
    scmp_filter_ctx ctx = seccomp_init(SCMP_ACT_ALLOW);
    if (!ctx) { 
        fprintf(stderr, "[sleepsize] seccomp_init falhou\n"); 
        return -1; 
    }
    
    /* ═══════════════════════════════════════════════════════════════════════
       BLOQUEIOS EXPLÍCITOS - Operações Perigosas
       ═══════════════════════════════════════════════════════════════════════ */
    
    /* Proteção contra exploração de kernel */
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(ptrace), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(process_vm_readv), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(process_vm_writev), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(perf_event_open), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(bpf), 0);
    
    /* Proteção contra escalação de privilégio */
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(chroot), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(pivot_root), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(unshare), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(kexec_load), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(kexec_file_load), 0);
    
    /* Acesso a módulos kernel */
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(init_module), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(delete_module), 0);
    seccomp_rule_add(ctx, SCMP_ACT_ERRNO(1), SCMP_SYS(finit_module), 0);
    
    int ret = seccomp_load(ctx);
    if (ret != 0) {
        fprintf(stderr, "[sleepsize] seccomp_load falhou: %m\n");
    } else {
        fprintf(stderr, "[sleepsize] Filtros Seccomp ativados (GTK3 compatible).\n");
    }
    seccomp_release(ctx);
    return ret;
}
