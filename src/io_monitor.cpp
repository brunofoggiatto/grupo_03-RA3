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
 * - O monitor de Rede lê o arquivo /proc/net/dev para obter estatísticas
 * de rede de todo o sistema.
 * -----------------------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm> // Para std::replace
#include <unistd.h>
#include "../include/monitor.hpp"

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: get_io_usage
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
 * - -1 se o arquivo /proc/[pid]/io não puder ser aberto (ex: processo
 * não existe ou permissão negada).
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

    // O arquivo /proc/[pid]/io tem o formato "chave: valor".
    // Nós lemos palavra por palavra, procurando as chaves que nos interessam.
    while (file >> key) {
        if (key == "read_bytes:") {
            file >> read_bytes;
        } else if (key == "write_bytes:") {
            file >> write_bytes;
        }
        // Outras chaves como 'rchar' (bytes lidos do cache) e
        // 'wchar' (bytes escritos no cache) são ignoradas.
    }

    file.close();

    // Armazena os valores encontrados no struct de estatísticas
    stats.io_read_bytes = read_bytes;
    stats.io_write_bytes = write_bytes;

    return 0;
}

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: calculate_io_rate
 *
 * DESCRIÇÃO:
 * Calcula a taxa de I/O (leitura e escrita) em bytes por segundo.
 *
 * COMO FUNCIONA:
 * Pega a diferença (delta) entre a coleta atual e a coleta anterior,
 * e divide pelo tempo (intervalo) que passou entre elas.
 *
 * PARÂMETROS:
 * - prev (const ProcStats&): O struct com os dados da coleta anterior.
 * - curr (ProcStats&): O struct com os dados da coleta atual. A taxa
 * calculada será armazenada aqui.
 * - interval (double): O tempo em segundos que passou entre 'prev' e 'curr'.
 * -----------------------------------------------------------------------------
 */
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    // Garante que não haja divisão por zero
    if (interval <= 0) interval = 1.0;

    curr.io_read_rate = (curr.io_read_bytes - prev.io_read_bytes) / interval;
    curr.io_write_rate = (curr.io_write_bytes - prev.io_write_bytes) / interval;

    // Se o processo for novo ou o contador do kernel "virar" (wraparound),
    // o delta pode ser negativo. Corrigimos isso para zero.
    if (curr.io_read_rate < 0) curr.io_read_rate = 0;
    if (curr.io_write_rate < 0) curr.io_write_rate = 0;
}

/*
 * -----------------------------------------------------------------------------
 * FUNÇÃO: get_network_usage
 *
 * DESCRIÇÃO:
 * Coleta as estatísticas de Rede (bytes recebidos/RX e transmitidos/TX)
 * de *todo o sistema*.
 *
 * COMO FUNCIONA:
 * Lê o arquivo /proc/net/dev, que sumariza o tráfego de todas as
 * interfaces de rede do sistema (ex: eth0, wlan0, etc.).
 *
 * IMPORTANTE:
 * Diferente do I/O, as estatísticas de rede no /proc não são
 * facilmente separadas por PID. Esta função mede o total do sistema,
 * o que é uma limitação conhecida desse método de monitoramento.
 *
 * PARÂMETROS:
 * - stats (ProcStats&): A referência ao struct onde os dados totais
 * (net_rx_bytes, net_tx_bytes) serão salvos.
 *
 * RETORNA:
 * - 0 em sucesso.
 * - -1 se /proc/net/dev não puder ser aberto.
 * -----------------------------------------------------------------------------
 */
int get_network_usage(ProcStats& stats) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        std::cerr << "Erro: não foi possível abrir /proc/net/dev" << std::endl;
        return -1;
    }

    std::string line;
    // As duas primeiras linhas de /proc/net/dev são cabeçalhos. Pulamos elas.
    std::getline(file, line);
    std::getline(file, line);

    long total_rx = 0, total_tx = 0;

    // Processa linha por linha (cada linha = 1 interface de rede)
    while (std::getline(file, line)) {
        // O formato da linha é "iface: bytes ...".
        // Trocamos o ':' por ' ' para facilitar o parse.
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream iss(line);

        std::string iface;
        long rx_bytes = 0, tx_bytes = 0;
        long skip_field; // Variável para pular colunas que não nos interessam

        // A estrutura de colunas de /proc/net/dev é:
        // 1: iface, 2: rx_bytes, 3-9: (outros), 10: tx_bytes
        
        iss >> iface >> rx_bytes; // Lê a interface e os bytes recebidos

        // Pula os 7 campos intermediários (pacotes, erros, etc.)
        for (int i = 0; i < 7; ++i) iss >> skip_field;
        
        iss >> tx_bytes; // Lê os bytes transmitidos

        // Ignoramos a interface 'lo' (loopback), pois ela só mede
        // tráfego interno do computador para ele mesmo (ex: 127.0.0.1).
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
 *
 * DESCRIÇÃO:
 * Calcula a taxa de tráfego de rede (RX e TX) em bytes por segundo.
 *
 * COMO FUNCIONA:
 * Pega a diferença (delta) entre a coleta atual e a coleta anterior,
 * e divide pelo tempo (intervalo) que passou entre elas.
 *
 * PARÂMETROS:
 * - prev (const ProcStats&): O struct com os dados da coleta anterior.
 * - curr (ProcStats&): O struct com os dados da coleta atual. A taxa
 * calculada será armazenada aqui.
 * - interval (double): O tempo em segundos que passou entre 'prev' e 'curr'.
 * -----------------------------------------------------------------------------
 */
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    // Garante que não haja divisão por zero
    if (interval <= 0) interval = 1.0;

    curr.net_rx_rate = (curr.net_rx_bytes - prev.net_rx_bytes) / interval;
    curr.net_tx_rate = (curr.net_tx_bytes - prev.net_tx_bytes) / interval;

    // Corrige taxas negativas (caso de reset do contador)
    if (curr.net_rx_rate < 0) curr.net_rx_rate = 0;
    if (curr.net_tx_rate < 0) curr.net_tx_rate = 0;
}