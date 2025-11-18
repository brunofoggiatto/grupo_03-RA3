/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: io_monitor.cpp
 * ATUALIZADO: - Tratamento específico de erros
 * -----------------------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <sstream>
#include <string>
#include <algorithm>
#include <unistd.h>
#include <errno.h>
#include <cstring>
#include "../include/monitor.hpp"

// INCLUDES ADICIONADOS PELA TAREFA 2
#include <dirent.h>
#include <cstring>
#include <stdio.h>

// Códigos de erro semânticos para tratamento consistente
#define ERR_PROCESS_NOT_FOUND -2
#define ERR_PERMISSION_DENIED -3
#define ERR_UNKNOWN -4

// Obtém estatísticas de I/O de um processo específico
int get_io_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream file(path);
    if (!file.is_open()) {
        // Tratamento específico de erros com códigos semânticos
        if (errno == ENOENT) {
            std::cerr << "ERRO: Processo com PID " << pid << " não existe" << std::endl;
            return ERR_PROCESS_NOT_FOUND;
        } else if (errno == EACCES) {
            std::cerr << "ERRO: Permissão negada para acessar processo " << pid << std::endl;
            return ERR_PERMISSION_DENIED;
        } else {
            std::cerr << "ERRO: Não foi possível abrir " << path << " - " << strerror(errno) << std::endl;
            return ERR_UNKNOWN;
        }
    }

    long read_bytes = 0, write_bytes = 0;
    std::string key;

    // Parse do arquivo /proc/[pid]/io para extrair métricas de I/O
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

// Calcula taxas de I/O com base na diferença entre leituras consecutivas
void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.io_read_rate = (curr.io_read_bytes - prev.io_read_bytes) / interval;
    curr.io_write_rate = (curr.io_write_bytes - prev.io_write_bytes) / interval;
    // Garante que taxas não sejam negativas (em caso de reset de contadores)
    if (curr.io_read_rate < 0) curr.io_read_rate = 0;
    if (curr.io_write_rate < 0) curr.io_write_rate = 0;
}

// Obtém estatísticas de rede do sistema inteiro
int get_network_usage(ProcStats& stats) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir /proc/net/dev - " << strerror(errno) << std::endl;
        return ERR_UNKNOWN;
    }
    std::string line;
    std::getline(file, line);  // Pula cabeçalhos
    std::getline(file, line);

    long total_rx = 0, total_tx = 0;
    while (std::getline(file, line)) {
        // Substitui ':' por espaço para facilitar parsing
        std::replace(line.begin(), line.end(), ':', ' ');
        std::istringstream iss(line);
        std::string iface;
        long rx_bytes = 0, tx_bytes = 0;
        long skip_field;
        
        iss >> iface >> rx_bytes;
        // Pula campos intermediários (packets, errors, etc.)
        for (int i = 0; i < 7; ++i) iss >> skip_field;
        iss >> tx_bytes;

        // Ignora interface loopback para estatísticas de rede real
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

// Calcula taxas de rede com base na diferença entre leituras consecutivas
void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.net_rx_rate = (curr.net_rx_bytes - prev.net_rx_bytes) / interval;
    curr.net_tx_rate = (curr.net_tx_bytes - prev.net_tx_bytes) / interval;
    // Garante que taxas não sejam negativas
    if (curr.net_rx_rate < 0) curr.net_rx_rate = 0;
    if (curr.net_tx_rate < 0) curr.net_tx_rate = 0;
}

// Obtém número de conexões TCP de um processo específico
int get_network_usage(int pid, ProcStats& stats) {
    stats.tcp_connections = 0;
    
    std::ifstream tcp("/proc/net/tcp");
    if (!tcp.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir /proc/net/tcp - " << strerror(errno) << std::endl;
        return ERR_UNKNOWN;
    }
    
    std::string line;
    std::getline(tcp, line);  // Pula cabeçalho

    // Parse de cada linha do /proc/net/tcp
    while (std::getline(tcp, line)) {
        unsigned long inode;
        
        // Extrai inode da conexão TCP (último campo)
        if (sscanf(line.c_str(), "%*d: %*s %*s %*s %*s %*s %*s %*s %*s %lu", &inode) == 1) {
            
            // Verifica se processo tem file descriptor apontando para este socket
            std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
            DIR* dir = opendir(fd_path.c_str());
            if (!dir) {
                if (errno == ENOENT) {
                    return ERR_PROCESS_NOT_FOUND;
                } else if (errno == EACCES) {
                    return ERR_PERMISSION_DENIED;
                }
                continue;
            }

            struct dirent* entry;
            // Itera sobre todos os file descriptors do processo
            while ((entry = readdir(dir)) != nullptr) {
                char link[256];
                std::string link_path = fd_path + "/" + entry->d_name;
                
                // Lê o link simbólico do file descriptor
                ssize_t len = readlink(link_path.c_str(), link, sizeof(link)-1);
                
                if (len > 0) {
                    link[len] = '\0';
                    std::string socket_id = "socket:[" + std::to_string(inode) + "]";
                    // Verifica se é o socket que estamos procurando
                    if (strstr(link, socket_id.c_str())) {
                        stats.tcp_connections++;
                        break;
                    }
                }
            }
            closedir(dir);
        }
    }
    return 0;
}

// Sobrecarga da função para estrutura NetworkStats específica
int get_network_usage(int pid, NetworkStats& stats) {
    std::ifstream tcp("/proc/net/tcp");
    if (!tcp.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir /proc/net/tcp - " << strerror(errno) << std::endl;
        return ERR_UNKNOWN;
    }
    std::string line;
    std::getline(tcp, line);

    while (std::getline(tcp, line)) {
        unsigned long inode;
        
        if (sscanf(line.c_str(), "%*d: %*s %*s %*s %*s %*s %*s %*s %*s %lu", &inode) == 1) {
            
            std::string fd_path = "/proc/" + std::to_string(pid) + "/fd";
            DIR* dir = opendir(fd_path.c_str());
            if (!dir) {
                if (errno == ENOENT) {
                    return ERR_PROCESS_NOT_FOUND;
                } else if (errno == EACCES) {
                    return ERR_PERMISSION_DENIED;
                }
                continue;
            }

            struct dirent* entry;
            while ((entry = readdir(dir)) != nullptr) {
                char link[256];
                std::string link_path = fd_path + "/" + entry->d_name;
                
                ssize_t len = readlink(link_path.c_str(), link, sizeof(link)-1);
                
                if (len > 0) {
                    link[len] = '\0';
                    std::string socket_id = "socket:[" + std::to_string(inode) + "]";
                    if (strstr(link, socket_id.c_str())) {
                        stats.tcp_connections++;
                        break;
                    }
                }
            }
            closedir(dir);
        }
    }
    return 0;
}