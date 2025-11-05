#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include "monitor.h"




// ---------------------------------------------------
// 2) Coleta o uso de memória física (RSS) do processo
// ----------------------------------------------------
int get_memory_usage(int pid, ProcStats *stats) {
    char path[64], line[256];                   // variáveis pra guardar caminho e linhas lidas
    snprintf(path, sizeof(path), "/proc/%d/status", pid);  // monta o caminho do arquivo

    FILE *f = fopen(path, "r");                 // abre o arquivo /proc/[pid]/status
    if (!f) return -1;                          // se não abrir, retorna erro

    // percorre o arquivo linha por linha
    while (fgets(line, sizeof(line), f)) {
        // procura pela linha que começa com "VmRSS:"
        if (strncmp(line, "VmRSS:", 6) == 0) {
            // extrai o número da linha e salva em stats->memory_rss
            sscanf(line, "VmRSS: %ld", &stats->memory_rss);
            break;  // achou o valor, pode sair do loop
        }
    }

    fclose(f);  // fecha o arquivo
    return 0;   // sucesso
}
