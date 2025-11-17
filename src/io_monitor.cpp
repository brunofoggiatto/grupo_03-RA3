/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: io_monitor.cpp
 * ATUALIZADO: 2025-11-17 - Tratamento específico de erros
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

// Códigos de erro semânticos
#define ERR_PROCESS_NOT_FOUND -2
#define ERR_PERMISSION_DENIED -3
#define ERR_UNKNOWN -4

int get_io_usage(int pid, ProcStats& stats) {
    std::string path = "/proc/" + std::to_string(pid) + "/io";
    std::ifstream file(path);
    if (!file.is_open()) {
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

void calculate_io_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.io_read_rate = (curr.io_read_bytes - prev.io_read_bytes) / interval;
    curr.io_write_rate = (curr.io_write_bytes - prev.io_write_bytes) / interval;
    if (curr.io_read_rate < 0) curr.io_read_rate = 0;
    if (curr.io_write_rate < 0) curr.io_write_rate = 0;
}

int get_network_usage(ProcStats& stats) {
    std::ifstream file("/proc/net/dev");
    if (!file.is_open()) {
        std::cerr << "ERRO: Não foi possível abrir /proc/net/dev - " << strerror(errno) << std::endl;
        return ERR_UNKNOWN;
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

void calculate_network_rate(const ProcStats& prev, ProcStats& curr, double interval) {
    if (interval <= 0) interval = 1.0;
    curr.net_rx_rate = (curr.net_rx_bytes - prev.net_rx_bytes) / interval;
    curr.net_tx_rate = (curr.net_tx_bytes - prev.net_tx_bytes) / interval;
    if (curr.net_rx_rate < 0) curr.net_rx_rate = 0;
    if (curr.net_tx_rate < 0) curr.net_tx_rate = 0;
}

int get_network_usage(int pid, ProcStats& stats) {
    stats.tcp_connections = 0;
    
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