#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include "../include/monitor.hpp"

// Coleta de métricas de I/O (leitura e escrita em disco) e Rede (bytes RX/TX)

// --- Coleta de I/O ---
int get_io_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream file(path);
    if (!file.is_open()) {
        std::cerr << "Erro: não foi possível abrir " << path << std::endl;
        return -1;
    }

    long read_bytes = 0, write_bytes = 0;
    std::string key;

    // Percorre as linhas do /proc/[pid]/io
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

// --- Cálculo da taxa de I/O ---
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;

    curr.io_read_rate = (curr.io_read_bytes - prev.io_read_bytes) / interval;
    curr.io_write_rate = (curr.io_write_bytes - prev.io_write_bytes) / interval;

    // Evita valores negativos (caso de reset do contador)
    if (curr.io_read_rate < 0) curr.io_read_rate = 0;
    if (curr.io_write_rate < 0) curr.io_write_rate = 0;
}

// --- Coleta de métricas de rede ---
int get_network_usage(ProcStats& stats) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        std::cerr << "Erro: não foi possível abrir /proc/net/dev" << std::endl;
        return -1;
    }

    std::string line;
    std::getline(file, line); // cabeçalho
    std::getline(file, line); // cabeçalho

    long total_rx = 0, total_tx = 0;

    // Percorre interfaces de rede
    while (std::getline(file, line)) {
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream iss(line);

        std::string iface;
        long rx_bytes = 0, tx_bytes = 0;
        long skip;

        // Estrutura padrão: iface, rx_bytes, rx_packets, ..., tx_bytes (colunas 9+)
        iss >> iface >> rx_bytes;
        // pula 7 campos até chegar no tx_bytes
        for (int i = 0; i < 7; ++i) iss >> skip;
        iss >> tx_bytes;

        // Ignora interfaces virtuais de loopback
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

// --- Cálculo da taxa de rede ---
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;

    curr.net_rx_rate = (curr.net_rx_bytes - prev.net_rx_bytes) / interval;
    curr.net_tx_rate = (curr.net_tx_bytes - prev.net_tx_bytes) / interval;

    // Evita negativos
    if (curr.net_rx_rate < 0) curr.net_rx_rate = 0;
    if (curr.net_tx_rate < 0) curr.net_tx_rate = 0;
}