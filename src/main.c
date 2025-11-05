#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <unistd.h>
#include <time.h>
#include "monitor.h"

int main(int argc, char *argv[]) {
    // Verifica se os parâmetros foram passados
    if (argc < 4) {
        printf("Uso: %s profile <PID> <intervalo_segundos>\n", argv[0]);
        return 1;
    }

    // Verifica se o comando é "profile"
    if (strcmp(argv[1], "profile") != 0) {
        printf("Comando inválido! Use: profile\n");
        return 1;
    }

    int pid = atoi(argv[2]);       // PID do processo
    int intervalo = atoi(argv[3]); // Intervalo entre coletas

    // Verifica se o processo existe
    char path[64];
    snprintf(path, sizeof(path), "/proc/%d", pid);
    if (access(path, F_OK) != 0) {
        printf("[ERRO] Processo %d não encontrado!\n", pid);
        return 1;
    }

    printf("Monitorando processo PID %d a cada %d segundos...\n", pid, intervalo);
    printf("Pressione Ctrl+C para parar.\n\n");

    ProcStats stats;

    while (1) {
        // Coleta CPU e Memória
        int cpu_ok = get_cpu_usage(pid, &stats);
        int mem_ok = get_memory_usage(pid, &stats);

        if (cpu_ok == 0 && mem_ok == 0) {
            time_t agora = time(NULL);
            struct tm *tm_info = localtime(&agora);
            char horario[32];
            strftime(horario, sizeof(horario), "%H:%M:%S", tm_info);

            printf("[%s] CPU: %.2f%% | Memória: %ld kB\n",
                   horario, stats.cpu_usage, stats.memory_rss);
        } else {
            printf("[ERRO] Falha ao coletar dados do processo %d\n", pid);
        }

        sleep(intervalo);
    }

    return 0;
}
