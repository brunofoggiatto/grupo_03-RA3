/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: io_monitor.cpp
 *
 * PROPÓSITO:
 * Este arquivo implementa as funções de monitoramento de I/O (disco) e Rede,
 * como parte do Componente 1 (Resource Profiler).
 *
 * RESPONSABILIDADE:
 * Aluno 2.
 *
 * NOTAS DE IMPLEMENTAÇÃO:
 * - O monitor de I/O lê o arquivo /proc/[pid]/io para obter estatísticas
 * de I/O de disco por processo.
 * - O monitor de Rede (versão 1) lê /proc/net/dev para estatísticas
 * de rede de todo o sistema (taxa de bytes).
 * - O monitor de Rede (versão 2) lê /proc/net/tcp e /proc/[pid]/fd
 * para "caçar" conexões TCP por processo.
 * -----------------------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // Para std::replace
#include <unistd.h>
#include "../include/monitor.hpp"

// INCLUDES ADICIONADOS PELA TAREFA 2
#include <dirent.h>  // Para opendir, readdir, closedir
#include <cstring>   // Para strstr
#include <stdio.h>   // Para sscanf

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: get_io_usage
 * (Função original - I/O por processo)
 *
 * DESCRIÇÃO:
 * Coleta as estatísticas de I/O (bytes lidos e escritos) para um processo.
 *
 * COMO FUNCIONA:
 * Lê o arquivo virtual /proc/[pid]/io. Este arquivo é mantido pelo Kernel
 * e contém contadores de I/O de *disco* reais.
 *
 * IMPORTANTE:
 * Estes contadores (read_bytes, write_bytes) medem o I/O que
 * realmente foi para o dispositivo de armazenamento (disco, SSD), e não
 * o I/O que foi servido pelo cache de página (RAM).
 *
 * PARÂMETROS:
 * - pid (int): O ID do processo que queremos monitorar.
 * - stats (ProcStats&): A referência ao struct onde os dados coletados
 * (io_read_bytes, io_write_bytes) serão salvos.
 *
 * RETORNA:
 * - 0 em sucesso.
 * - -1 se o arquivo /proc/[pid]/io não puder ser aberto.
 * -----------------------------------------------------------------------------
 */
int get_io_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro: não foi possível abrir " << path << std::endl;
        return -1;
    }

    long read_bytes = 0, write_bytes = 0;
    std::string key;

    while (file >> key) {
        if (key == "read_bytes:") {
            file >> read_bytes;
        } else if (key == "write_bytes:") {
            file >> write_bytes;
        }
    }
    file.close();

    stats.io_read_bytes = read_bytes;
    stats.io_write_bytes = write_bytes;

    return 0;
}

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: calculate_io_rate
 * (Função original)
 *
 * DESCRIÇÃO:
 * Calcula a taxa de I/O (leitura e escrita) em bytes por segundo.
 *
 * ... (Restante do comentário e função como no seu arquivo) ...
 * -----------------------------------------------------------------------------
 */
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.io_read_rate = (curr.io_read_bytes - prev.io_read_bytes) / interval;
    curr.io_write_rate = (curr.io_write_bytes - prev.io_write_bytes) / interval;
    if (curr.io_read_rate < 0) curr.io_read_rate = 0;
    if (curr.io_write_rate < 0) curr.io_write_rate = 0;
}

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: get_network_usage (Versão 1 - Sistema Inteiro)
 * (Função original)
 *
 * DESCRIÇÃO:
 * Coleta as estatísticas de Rede (bytes recebidos/RX e transmitidos/TX)
 * de *todo o sistema*.
 *
 * ... (Restante do comentário e função como no seu arquivo) ...
 * -----------------------------------------------------------------------------
 */
int get_network_usage(ProcStats& stats) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        std::cerr << "Erro: não foi possível abrir /proc/net/dev" << std::endl;
        return -1;
    }
    std::string line;
    std::getline(file, line);
    std::getline(file, line);

    long total_rx = 0, total_tx = 0;
    while (std::getline(file, line)) {
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream iss(line);
        std::string iface;
        long rx_bytes = 0, tx_bytes = 0;
        long skip_field;
        
        iss >> iface >> rx_bytes;
        for (int i = 0; i < 7; ++i) iss >> skip_field;
        iss >> tx_bytes;

        if (iface != "lo") {
            total_rx += rx_bytes;
            total_tx += tx_bytes;
        }
    }
    file.close();
    stats.net_rx_bytes = total_rx;
    stats.net_tx_bytes = total_tx;
    return 0;
}

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: calculate_network_rate
 * (Função original)
 *
 * DESCRIÇÃO:
 * Calcula a taxa de tráfego de rede (RX e TX) em bytes por segundo.
 *
 * ... (Restante do comentário e função como no seu arquivo) ...
 * -----------------------------------------------------------------------------
 */
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.net_rx_rate = (curr.net_rx_bytes - prev.net_rx_bytes) / interval;
    curr.net_tx_rate = (curr.net_tx_bytes - prev.net_tx_bytes) / interval;
    if (curr.net_rx_rate < 0) curr.net_rx_rate = 0;
    if (curr.net_tx_rate < 0) curr.net_tx_rate = 0;
}


/*
 * =============================================================================
 * INÍCIO DO CÓDIGO DA TAREFA 2 (PLANO ALUNO B)
 * =============================================================================
 */

/*
 * -----------------------------------------------------------------------------
 * STRUCT: NetworkStats
 *
 * DESCRIÇÃO:
 * Um novo struct, separado do ProcStats, para armazenar as métricas de
 * rede POR PROCESSO (neste caso, contagem de conexões).
 * -----------------------------------------------------------------------------
 */
struct NetworkStats {
    int tcp_connections = 0;
    int udp_connections = 0;
    // (A contagem de UDP pode ser implementada no futuro de forma similar)
};

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: get_network_usage (Versão 2 - Por Processo)
 *
 * DESCRIÇÃO:
 * Coleta o número de conexões TCP ativas para um PROCESSO específico.
 *
 * COMO FUNCIONA:
 * Este é um processo complexo de "cruzamento de dados" do /proc:
 * 1. Lê o arquivo `/proc/net/tcp`. Este arquivo lista TODOS os sockets
 * TCP ativos no sistema e seus 'inodes' (IDs únicos de kernel).
 * 2. Para CADA socket encontrado, ele "caça" esse inode dentro da pasta
 * de file descriptors (arquivos abertos) do processo: `/proc/[pid]/fd/`.
 * 3. Ele lê cada link simbólico nessa pasta (ex: /proc/123/fd/5).
 * 4. Se um link apontar para "socket:[INODE_PROCURADO]", significa que
 * o processo 123 é o dono daquele socket.
 * 5. Incrementa a contagem de conexões TCP.
 *
 * IMPORTANTE:
 * Esta é uma função C++ "sobrecarregada" (overloaded). Ela tem o
 * mesmo nome da função anterior, mas parâmetros diferentes. O compilador
 * sabe qual chamar com base no que você passa para ela.
 *
 * PARÂMETROS:
 * - pid (int): O ID do processo que queremos investigar.
 * - stats (NetworkStats&): O novo struct onde a contagem de conexões
 * será armazenada.
 *
 * RETORNA:
 * - 0 em sucesso.
 * - (Não retorna -1, mas 'continue' é usado se o /proc/[pid]/fd
 * não puder ser aberto, ex: processo morreu no meio da verificação).
 * -----------------------------------------------------------------------------
 */
int get_network_usage(int pid, NetworkStats& stats) {
    // Abre o arquivo que lista todos os sockets TCP do sistema
    std::ifstream tcp("/proc/net/tcp");
    std::string line;
    std::getline(tcp, line); // Pula a linha do cabeçalho

    // Processa cada socket ativo no sistema
    while (std::getline(tcp, line)) {
        unsigned long inode;
        
        // sscanf "pula" as 9 primeiras colunas e lê a 10ª (inode)
        // O formato %*s ignora um campo. %lu lê um unsigned long (o inode).
        if (sscanf(line.c_str(), "%*d: %*s %*s %*s %*s %*s %*s %*s %*s %lu", &inode) == 1) {
            
            // Agora, "caçamos" este inode no processo alvo (pid)
            std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
            DIR* dir = opendir(fd_path.c_str());
            if (!dir) continue; // Processo pode ter morrido, pula para o próximo socket

            struct dirent* entry;
            // Itera sobre todos os arquivos abertos (file descriptors) do processo
            while ((entry = readdir(dir)) != nullptr) {
                char link[256]; // Buffer para onde o link simbólico aponta
                std::string link_path = fd_path + "/" + entry->d_name;
                
                // readlink lê o destino do link (ex: /proc/123/fd/3 -> "socket:[4567]")
                ssize_t len = readlink(link_path.c_str(), link, sizeof(link)-1);
                
                if (len > 0) {
                    link[len] = '\0'; // Garante que a string termina
                    
                    // Compara se o link é o socket que estamos procurando
                    std::string socket_id = "socket:[" + std::to_string(inode) + "]";
                    if (strstr(link, socket_id.c_str())) {
                        stats.tcp_connections++;
                        break; // Achamos! Pare de procurar nos 'fd' deste processo
                    }
                }
            }
            closedir(dir);
        }
    }
    return 0;
}