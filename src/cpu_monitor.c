#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "monitor.h"

// ----------------------------------------------------
// 1) Coleta o tempo de CPU do processo em /proc/[pid]/stat
// ----------------------------------------------------
int get_cpu_usage(int pid, ProcStats *stats) {
    char path[64];                     // string que guardará o caminho /proc/[pid]/stat
    snprintf(path, sizeof(path), "/proc/%d/stat", pid); // monta o caminho usando o PID

    FILE *f = fopen(path, "r");        // abre o arquivo /proc/[pid]/stat
    if (!f) return -1;                 // se não conseguir abrir, retorna erro

    long utime, stime;                 // tempos de CPU: usuário e kernel
    char buffer[256];                  // usado pra pular campos que não interessam

    // O arquivo /proc/[pid]/stat tem mais de 50 campos, separados por espaço.
    // Os dois que interessam (utime e stime) estão nas posições 14 e 15.
    // Por isso, pulamos os 13 primeiros.
    for (int i = 0; i < 13; i++)
        fscanf(f, "%s", buffer);       // lê e descarta

    // Agora lemos utime e stime
    fscanf(f, "%ld %ld", &utime, &stime);

    fclose(f);                         // fecha o arquivo

    // Soma os dois tempos e armazena em cpu_usage
    // (ainda é um valor bruto, não porcentagem)
    stats->cpu_usage = (utime + stime) / 100.0;

    return 0;                          // sucesso
}
