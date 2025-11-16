#include "../include/namespace.hpp"
#include <iostream>
#include <fstream>
#include <sstream>
#include <filesystem>
#include <array>
#include <unistd.h>
#include <dirent.h>
#include <cstring>
#include <cstdio>

namespace fs = std::filesystem;

// Array com os nomes dos namespaces (na ordem do enum)
static constexpr std::array<const char*, 7> ns_names = {
    "cgroup", "ipc", "mnt", "net", "pid", "user", "uts"
};

std::string namespace_type_to_string(NamespaceType type) {
    auto idx = static_cast<size_t>(type);
    if (idx < ns_names.size()) {
        return ns_names[idx];
    }
    return "";
}

std::optional<NamespaceType> string_to_namespace_type(const std::string& str) {
    for (size_t i = 0; i < ns_names.size(); i++) {
        if (str == ns_names[i]) {
            return static_cast<NamespaceType>(i);
        }
    }
    return std::nullopt;
}

// Helper para obter nome do processo
static std::string get_process_name(pid_t pid) {
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    std::ifstream file(path);
    if (!file.is_open()) {
        return "";
    }

    std::string name;
    std::getline(file, name);
    return name;
}

// FUNÇÃO 1: Lista todos os namespaces de um processo
std::optional<ProcessNamespaces> list_process_namespaces(pid_t pid) {
    ProcessNamespaces proc_ns;
    proc_ns.pid = pid;
    proc_ns.ns_count = 0;
    proc_ns.process_name = get_process_name(pid);

    // pra cada tipo de namespace, tenta ler o link em /proc/[pid]/ns/
    for (size_t i = 0; i < ns_names.size(); i++) {
        NamespaceInfo ns_info;
        ns_info.type = static_cast<NamespaceType>(i);
        ns_info.name = ns_names[i];

        char ns_path[256];
        snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid, ns_names[i]);

        char link_buf[256];
        ssize_t len = readlink(ns_path, link_buf, sizeof(link_buf) - 1);

        if (len != -1) {
            link_buf[len] = '\0';
            ns_info.link = link_buf;

            // extrair o inode do formato "tipo:[numero]"
            unsigned long inode = 0;
            if (sscanf(link_buf, "%*[^[][%lu]", &inode) == 1) {
                ns_info.inode = inode;
            }

            ns_info.exists = true;
            proc_ns.ns_count++;
        } else {
            ns_info.exists = false;
        }

        proc_ns.namespaces.push_back(ns_info);
    }

    // se não conseguiu ler nenhum namespace, provavelmente o processo não existe
    if (proc_ns.ns_count == 0) {
        return std::nullopt;
    }

    return proc_ns;
}

// helper pra verificar se um path é um diretório de processo
static bool is_pid_dir(const std::string& name) {
    for (char c : name) {
        if (c < '0' || c > '9') {
            return false;
        }
    }
    return !name.empty();
}

// FUNÇÃO 2: Encontra processos no mesmo namespace
std::vector<pid_t> find_processes_in_namespace(NamespaceType ns_type, ino_t ns_inode) {
    std::vector<pid_t> pids;

    auto type_idx = static_cast<size_t>(ns_type);
    if (type_idx >= ns_names.size()) {
        return pids;
    }

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return pids;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        std::string dir_name = entry->d_name;

        if (!is_pid_dir(dir_name)) {
            continue;
        }

        pid_t pid = std::stoi(dir_name);
        char ns_path[256];
        snprintf(ns_path, sizeof(ns_path), "/proc/%s/ns/%s", dir_name.c_str(), ns_names[type_idx]);

        char link_buf[256];
        ssize_t len = readlink(ns_path, link_buf, sizeof(link_buf) - 1);

        if (len == -1) {
            continue;
        }

        link_buf[len] = '\0';
        unsigned long inode = 0;
        if (sscanf(link_buf, "%*[^[][%lu]", &inode) == 1) {
            if (inode == ns_inode) {
                pids.push_back(pid);
            }
        }
    }

    closedir(proc_dir);
    return pids;
}

// FUNÇÃO 3: Compara namespaces de dois processos
std::optional<NamespaceComparison> compare_namespaces(pid_t pid1, pid_t pid2) {
    auto proc1 = list_process_namespaces(pid1);
    auto proc2 = list_process_namespaces(pid2);

    if (!proc1 || !proc2) {
        return std::nullopt;
    }

    NamespaceComparison comparison;
    comparison.pid1 = pid1;
    comparison.pid2 = pid2;

    for (size_t i = 0; i < ns_names.size(); i++) {
        if (proc1->namespaces[i].exists && proc2->namespaces[i].exists) {
            comparison.total_namespaces++;

            if (proc1->namespaces[i].inode == proc2->namespaces[i].inode) {
                comparison.shared_namespaces++;
            } else {
                comparison.different_namespaces++;
                comparison.differences.push_back(static_cast<NamespaceType>(i));
            }
        }
    }

    return comparison;
}

// FUNÇÃO 4: Gera relatório do sistema
bool generate_namespace_report(const std::string& output_file, const std::string& format) {
    std::ofstream fp(output_file);
    if (!fp.is_open()) {
        return false;
    }

    bool is_json = (format == "json");

    if (is_json) {
        fp << "{\n  \"namespaces\": [\n";
    } else {
        fp << "Type,Inode,ProcessCount,PIDs\n";
    }

    // estrutura pra guardar namespaces únicos
    struct UniqueNS {
        NamespaceType type;
        ino_t inode;
        int count = 0;
    };

    std::vector<UniqueNS> unique_list;

    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        fp.close();
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        std::string dir_name = entry->d_name;

        if (!is_pid_dir(dir_name)) {
            continue;
        }

        pid_t pid = std::stoi(dir_name);
        auto proc_ns = list_process_namespaces(pid);

        if (!proc_ns) {
            continue;
        }

        for (const auto& ns : proc_ns->namespaces) {
            if (!ns.exists) {
                continue;
            }

            // verifica se ja existe na lista
            bool found = false;
            for (auto& unique : unique_list) {
                if (unique.type == ns.type && unique.inode == ns.inode) {
                    unique.count++;
                    found = true;
                    break;
                }
            }

            if (!found) {
                UniqueNS new_ns;
                new_ns.type = ns.type;
                new_ns.inode = ns.inode;
                new_ns.count = 1;
                unique_list.push_back(new_ns);
            }
        }
    }

    closedir(proc_dir);

    // escreve os resultados
    for (size_t i = 0; i < unique_list.size(); i++) {
        if (is_json) {
            fp << "    {\n";
            fp << "      \"type\": \"" << namespace_type_to_string(unique_list[i].type) << "\",\n";
            fp << "      \"inode\": " << unique_list[i].inode << ",\n";
            fp << "      \"process_count\": " << unique_list[i].count << "\n";
            fp << "    }" << (i < unique_list.size() - 1 ? "," : "") << "\n";
        } else {
            fp << namespace_type_to_string(unique_list[i].type) << ","
               << unique_list[i].inode << "," << unique_list[i].count << ",\n";
        }
    }

    if (is_json) {
        fp << "  ]\n}\n";
    }

    fp.close();
    return true;
}

// Funções auxiliares de print
void print_process_namespaces(const ProcessNamespaces& proc_ns) {
    std::cout << "\nNamespaces do processo PID " << proc_ns.pid;
    if (!proc_ns.process_name.empty()) {
        std::cout << " (" << proc_ns.process_name << ")";
    }
    std::cout << ":\n";
    std::cout << "----------------------------------------\n";

    for (const auto& ns : proc_ns.namespaces) {
        if (ns.exists) {
            printf("  %-8s: %s (inode: %lu)\n",
                   ns.name.c_str(), ns.link.c_str(), ns.inode);
        }
    }

    std::cout << "Total: " << proc_ns.ns_count << " namespaces\n";
}

void print_namespace_comparison(const NamespaceComparison& comparison) {
    std::cout << "\nComparacao entre PID " << comparison.pid1
              << " e PID " << comparison.pid2 << ":\n";
    std::cout << "----------------------------------------\n";
    std::cout << "Total de namespaces verificados: " << comparison.total_namespaces << "\n";
    std::cout << "Namespaces compartilhados: " << comparison.shared_namespaces << "\n";
    std::cout << "Namespaces diferentes: " << comparison.different_namespaces << "\n";

    if (comparison.different_namespaces > 0) {
        std::cout << "\nNamespaces que diferem:\n";
        for (const auto& diff : comparison.differences) {
            std::cout << "  - " << namespace_type_to_string(diff) << "\n";
        }
    }
}
