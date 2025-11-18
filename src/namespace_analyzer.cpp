// ============================================================
// ARQUIVO: src/namespace_analyzer.cpp
// DESCRIÇÃO: Implementação do Namespace Analyzer (Componente 2 - Aluno 3)
// Analisa e compara isolamento de processos usando namespaces do kernel
// Lê de /proc/[pid]/ns/ para extrair informações de isolamento
// ============================================================

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

// ================================
// ARRAY GLOBAL: ns_names
// Propósito: Mapear índices do enum para nomes de string
// Permite conversão bidirecional NamespaceType <-> string
// ================================
static constexpr std::array<const char*, 7> ns_names = {
    "cgroup",  // NamespaceType::CGROUP = 0 (Índice 0)
    "ipc",     // NamespaceType::IPC = 1 (Índice 1)
    "mnt",     // NamespaceType::MNT = 2 (Índice 2)
    "net",     // NamespaceType::NET = 3 (Índice 3)
    "pid",     // NamespaceType::PID = 4 (Índice 4)
    "user",    // NamespaceType::USER = 5 (Índice 5)
    "uts"      // NamespaceType::UTS = 6 (Índice 6)
};

// ================================
// FUNÇÃO: namespace_type_to_string
// Propósito: Converte enum para string
// Exemplo: NamespaceType::PID -> "pid"
// Parâmetros:
//   type: valor do enum NamespaceType
// Retorno: string com nome do namespace
// ================================
std::string namespace_type_to_string(NamespaceType type) {
    // Converte enum (que é um integer) para índice do array
    auto idx = static_cast<size_t>(type);
    
    // Verifica se índice é válido
    if (idx < ns_names.size()) {
        return ns_names[idx];  // Retorna nome correspondente
    }
    return "";  // Retorna string vazia se inválido
}

// ================================
// FUNÇÃO: string_to_namespace_type
// Propósito: Converte string para enum
// Inverso de namespace_type_to_string
// Exemplo: "pid" -> NamespaceType::PID
// Parâmetros:
//   str: string com nome do namespace
// Retorno: std::optional com enum, ou std::nullopt se não encontrado
// ================================
std::optional<NamespaceType> string_to_namespace_type(const std::string& str) {
    // Itera sobre todos os nomes definidos
    for (size_t i = 0; i < ns_names.size(); i++) {
        // Se encontrou correspondência
        if (str == ns_names[i]) {
            // Retorna o enum correspondente
            return static_cast<NamespaceType>(i);
        }
    }
    // Não encontrou, retorna empty optional
    return std::nullopt;
}

// ================================
// FUNÇÃO AUXILIAR: get_process_name
// Propósito: Lê o nome do executável de um processo
// Lê de /proc/[pid]/comm que contém o nome do programa
// Exemplo: /proc/1234/comm contém "bash\n"
// Parâmetros:
//   pid: ID do processo
// Retorno: nome do executável, ou string vazia se erro
// ================================
static std::string get_process_name(pid_t pid) {
    // Constrói o caminho: /proc/[pid]/comm
    char path[256];
    snprintf(path, sizeof(path), "/proc/%d/comm", pid);

    std::ifstream file(path);
    if (!file.is_open()) {
        return "";  // Não conseguiu ler, retorna vazio
    }

    // Lê a primeira linha (nome do programa)
    std::string name;
    std::getline(file, name);
    return name;
}

// ================================
// FUNÇÃO PRINCIPAL 1: list_process_namespaces
// Propósito: Lista todos os 7 namespaces de um processo
// Lê os links simbólicos em /proc/[pid]/ns/
// Exemplo de link: cgroup -> cgroup:[4026531835]
// Parâmetros:
//   pid: ID do processo a analisar
// Retorno: std::optional com ProcessNamespaces (dados ou std::nullopt se erro)
// ================================
std::optional<ProcessNamespaces> list_process_namespaces(pid_t pid) {
    // Inicializa a estrutura para armazenar resultados
    ProcessNamespaces proc_ns;
    proc_ns.pid = pid;
    proc_ns.ns_count = 0;
    proc_ns.process_name = get_process_name(pid);  // Nome do processo

    // ================================
    // ITERAR SOBRE OS 7 TIPOS DE NAMESPACE
    // ================================
    for (size_t i = 0; i < ns_names.size(); i++) {
        // Cria estrutura para informações deste namespace
        NamespaceInfo ns_info;
        ns_info.type = static_cast<NamespaceType>(i);  // Tipo (ex: PID)
        ns_info.name = ns_names[i];                    // Nome (ex: "pid")

        // ================================
        // LER LINK SIMBÓLICO
        // ================================
        // Constrói o caminho: /proc/[pid]/ns/[tipo]
        char ns_path[256];
        snprintf(ns_path, sizeof(ns_path), "/proc/%d/ns/%s", pid, ns_names[i]);

        // Lê o link simbólico
        // Exemplo: /proc/1234/ns/pid -> "pid:[4026531836]"
        char link_buf[256];
        ssize_t len = readlink(ns_path, link_buf, sizeof(link_buf) - 1);

        if (len != -1) {
            // Sucesso ao ler o link
            link_buf[len] = '\0';  // Termina string com null
            ns_info.link = link_buf;  // Armazena conteúdo completo

            // ================================
            // EXTRAIR INODE DO LINK
            // ================================
            // Formato: "tipo:[numero]"
            // Exemplo: "pid:[4026531836]"
            // Queremos extrair só o número (4026531836)
            // O inode é o identificador único do namespace
            
            unsigned long inode = 0;
            // sscanf %*[^[] = pula até primeiro '['
            // %lu = lê unsigned long
            // %*] = ignora ']'
            if (sscanf(link_buf, "%*[^[][%lu]", &inode) == 1) {
                ns_info.inode = inode;  // Armazena inode
            }

            ns_info.exists = true;
            proc_ns.ns_count++;  // Incrementa contador
        } else {
            // Falha ao ler o link
            // Pode ser: permissão negada ou namespace não existe
            ns_info.exists = false;
        }

        // Adiciona à lista de namespaces
        proc_ns.namespaces.push_back(ns_info);
    }

    // ================================
    // VALIDAÇÃO
    // ================================
    // Se não conseguiu ler nenhum namespace,
    // provavelmente o processo não existe ou não tem permissão
    if (proc_ns.ns_count == 0) {
        return std::nullopt;  // Retorna empty optional
    }

    return proc_ns;  // Retorna dados válidos
}

// ================================
// FUNÇÃO AUXILIAR: is_pid_dir
// Propósito: Verifica se nome de diretório é um PID
// PIDs são sempre números
// Parâmetros:
//   name: nome do diretório (ex: "1234" ou "proc")
// Retorno: true se é um PID válido
// ================================
static bool is_pid_dir(const std::string& name) {
    // Verifica cada caractere
    for (char c : name) {
        // Se encontrou algo que não é dígito
        if (c < '0' || c > '9') {
            return false;  // Não é PID
        }
    }
    // Se chegou aqui e não está vazio, é um PID
    return !name.empty();
}

// ================================
// FUNÇÃO PRINCIPAL 2: find_processes_in_namespace
// Propósito: Encontra todos os processos em um namespace específico
// Procura em /proc/[1..max_pid]/ns/[tipo] pelo mesmo inode
// Exemplo: Encontrar todos os processos no namespace NET com inode 4026531956
// Parâmetros:
//   ns_type: tipo de namespace (ex: NET)
//   ns_inode: inode que identifica o namespace
// Retorno: vector com PIDs dos processos
// ================================
std::vector<pid_t> find_processes_in_namespace(NamespaceType ns_type, ino_t ns_inode) {
    std::vector<pid_t> pids;  // Resultados

    // Converte enum para índice do array
    auto type_idx = static_cast<size_t>(ns_type);
    if (type_idx >= ns_names.size()) {
        return pids;  // Retorna vazio se tipo inválido
    }

    // ================================
    // ABRIR /proc
    // ================================
    // Lista todos os PIDs (cada entrada é um diretório numerado)
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        return pids;  // Retorna vazio se erro
    }

    struct dirent* entry;
    // Lê cada entrada no diretório /proc
    while ((entry = readdir(proc_dir)) != nullptr) {
        std::string dir_name = entry->d_name;

        // ================================
        // FILTRAR DIRETÓRIOS DE PROCESSO
        // ================================
        // Ignora ., .., sys, net, etc
        // Apenas processa PIDs (números)
        if (!is_pid_dir(dir_name)) {
            continue;  // Pula se não é PID
        }

        // Converte nome para inteiro PID
        pid_t pid = std::stoi(dir_name);
        
        // ================================
        // LER NAMESPACE DO PROCESSO
        // ================================
        // Constrói caminho: /proc/[pid]/ns/[tipo]
        char ns_path[256];
        snprintf(ns_path, sizeof(ns_path), "/proc/%s/ns/%s", dir_name.c_str(), ns_names[type_idx]);

        // Lê o link simbólico
        char link_buf[256];
        ssize_t len = readlink(ns_path, link_buf, sizeof(link_buf) - 1);

        if (len == -1) {
            continue;  // Falha ao ler, pula
        }

        link_buf[len] = '\0';
        
        // ================================
        // EXTRAIR INODE E COMPARAR
        // ================================
        unsigned long inode = 0;
        if (sscanf(link_buf, "%*[^[][%lu]", &inode) == 1) {
            // Se o inode bate, é um processo no mesmo namespace
            if (inode == ns_inode) {
                pids.push_back(pid);
            }
        }
    }

    closedir(proc_dir);
    return pids;
}

// ================================
// FUNÇÃO PRINCIPAL 3: compare_namespaces
// Propósito: Compara namespaces de dois processos
// Verifica quais tipos compartilham (inode igual)
// Parâmetros:
//   pid1: ID do primeiro processo
//   pid2: ID do segundo processo
// Retorno: std::optional com NamespaceComparison
// ================================
std::optional<NamespaceComparison> compare_namespaces(pid_t pid1, pid_t pid2) {
    // Obtém namespaces de ambos os processos
    auto proc1 = list_process_namespaces(pid1);
    auto proc2 = list_process_namespaces(pid2);

    // Se algum não foi encontrado, retorna vazio
    if (!proc1 || !proc2) {
        return std::nullopt;
    }

    NamespaceComparison comparison;
    comparison.pid1 = pid1;
    comparison.pid2 = pid2;

    // ================================
    // COMPARAR CADA TIPO DE NAMESPACE
    // ================================
    for (size_t i = 0; i < ns_names.size(); i++) {
        // Se ambos têm este namespace
        if (proc1->namespaces[i].exists && proc2->namespaces[i].exists) {
            comparison.total_namespaces++;

            // Se os inodes são iguais, compartilham o namespace
            if (proc1->namespaces[i].inode == proc2->namespaces[i].inode) {
                comparison.shared_namespaces++;
            } else {
                // Se inodes diferem, têm namespaces diferentes
                comparison.different_namespaces++;
                comparison.differences.push_back(static_cast<NamespaceType>(i));
            }
        }
    }

    return comparison;
}

// ================================
// FUNÇÃO PRINCIPAL 4: generate_namespace_report
// Propósito: Gera um relatório completo do sistema
// Scanneia todos os processos e agrupa por namespace+inode
// Parâmetros:
//   output_file: caminho do arquivo de saída
//   format: "csv" ou "json"
// Retorno: true se sucesso, false se erro
// ================================
bool generate_namespace_report(const std::string& output_file, const std::string& format) {
    std::ofstream fp(output_file);
    if (!fp.is_open()) {
        return false;  // Não conseguiu abrir arquivo
    }

    bool is_json = (format == "json");

    // ================================
    // ESCREVER CABEÇALHO BASEADO EM FORMATO
    // ================================
    if (is_json) {
        fp << "{\n  \"namespaces\": [\n";
    } else {
        // CSV
        fp << "Type,Inode,ProcessCount,PIDs\n";
    }

    // ================================
    // ESTRUTURA PARA ARMAZENAR NAMESPACES ÚNICOS
    // ================================
    struct UniqueNS {
        NamespaceType type;
        ino_t inode;
        int count = 0;  // Quantos processos usam este namespace
    };

    std::vector<UniqueNS> unique_list;

    // ================================
    // ESCANEAR TODOS OS PROCESSOS
    // ================================
    DIR* proc_dir = opendir("/proc");
    if (!proc_dir) {
        fp.close();
        return false;
    }

    struct dirent* entry;
    while ((entry = readdir(proc_dir)) != nullptr) {
        std::string dir_name = entry->d_name;

        // Filtra apenas PIDs (números)
        if (!is_pid_dir(dir_name)) {
            continue;
        }

        pid_t pid = std::stoi(dir_name);
        
        // Obtém todos os namespaces deste processo
        auto proc_ns = list_process_namespaces(pid);

        if (!proc_ns) {
            continue;  // Processo não mais existe
        }

        // ================================
        // PROCESSAR CADA NAMESPACE DO PROCESSO
        // ================================
        for (const auto& ns : proc_ns->namespaces) {
            if (!ns.exists) {
                continue;  // Namespace não existe, pula
            }

            // ================================
            // VERIFICAR SE NAMESPACE JÁ ESTÁ LISTADO
            // ================================
            bool found = false;
            for (auto& unique : unique_list) {
                if (unique.type == ns.type && unique.inode == ns.inode) {
                    unique.count++;  // Incrementa contador
                    found = true;
                    break;
                }
            }

            // Se não encontrou, adiciona novo
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

    // ================================
    // ESCREVER RESULTADOS
    // ================================
    for (size_t i = 0; i < unique_list.size(); i++) {
        if (is_json) {
            // Formato JSON
            fp << "    {\n";
            fp << "      \"type\": \"" << namespace_type_to_string(unique_list[i].type) << "\",\n";
            fp << "      \"inode\": " << unique_list[i].inode << ",\n";
            fp << "      \"process_count\": " << unique_list[i].count << "\n";
            fp << "    }" << (i < unique_list.size() - 1 ? "," : "") << "\n";
        } else {
            // Formato CSV
            fp << namespace_type_to_string(unique_list[i].type) << ","
               << unique_list[i].inode << "," 
               << unique_list[i].count << ",\n";
        }
    }

    if (is_json) {
        fp << "  ]\n}\n";
    }

    fp.close();
    return true;  // Sucesso
}

// ================================
// FUNÇÕES AUXILIARES DE EXIBIÇÃO
// ================================

// ================================
// FUNÇÃO: print_process_namespaces
// Propósito: Imprime namespaces de um processo de forma formatada
// ================================
void print_process_namespaces(const ProcessNamespaces& proc_ns) {
    std::cout << "\nNamespaces do processo PID " << proc_ns.pid;
    if (!proc_ns.process_name.empty()) {
        std::cout << " (" << proc_ns.process_name << ")";  // Nome do executável
    }
    std::cout << ":\n";
    std::cout << "----------------------------------------\n";

    // Para cada namespace encontrado
    for (const auto& ns : proc_ns.namespaces) {
        if (ns.exists) {
            // Imprime: nome [5 chars], link completo, inode
            printf("  %-8s: %s (inode: %lu)\n",
                   ns.name.c_str(), ns.link.c_str(), ns.inode);
        }
    }

    std::cout << "Total: " << proc_ns.ns_count << " namespaces\n";
}

// ================================
// FUNÇÃO: print_namespace_comparison
// Propósito: Imprime comparação entre dois processos
// ================================
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