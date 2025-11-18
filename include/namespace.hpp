// ============================================================
// ARQUIVO: include/namespace.hpp
// DESCRIÇÃO: Headers do Namespace Analyzer (Componente 2)
// Contém definições para análise e comparação de namespaces
// do kernel Linux (/proc/[pid]/ns/)
// ============================================================

#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include <sys/types.h>  // Para pid_t, ino_t
#include <cstdint>      // Para tipos inteiros padrão
#include <string>       // Para std::string
#include <vector>       // Para std::vector
#include <optional>     // Para std::optional


enum class NamespaceType {
    // Isolamento de control groups
    // Isola visão de cgroups
    CGROUP,
    
    //Isolamento de IPC 
    // Isola semáforos, filas de mensagens, memória compartilhada
    IPC,
    
    //Isolamento de pontos de montagem 
    // Cada namespace tem sua própria view de filesystems montados
    MNT,
    
    //Isolamento de stack de rede
    // Cada namespace tem suas próprias interfaces, rotas, sockets
    NET,
    
    //Isolamento de identificadores de processos
    PID,
    
    //Isolamento de UIDs/GIDs
    // Permite mapear UIDs 
    USER,
    
    //Isolamento de hostname e domainname
    UTS,
    
    // Constante para indicar total de tipos 
    COUNT
};
// NamespaceInfo: informações de UM namespace específico
struct NamespaceInfo {
    // Tipo do namespace
    // Indica qual tipo de isolamento é este
    NamespaceType type;
    
    // Nome do tipo em string 
    // Usado para exibição e logging
    std::string name;
    
    // Número do inode 
    ino_t inode;
    
    // Conteúdo completo do link simbólico
    std::string link;
    
    // True se o namespace existe para este processo
    // False se não conseguiu ler
    bool exists;
};

// ProcessNamespaces: TODOS os 7 namespaces de um processo
struct ProcessNamespaces {
    // ID do processo que foi analisado
    pid_t pid;
    
    // Array com informações dos 7 tipos de namespaces
    std::vector<NamespaceInfo> namespaces;
    
    // Contagem de quantos namespaces estão ativos/existem
    int ns_count;
    
    // Nome do programa executável
    std::string process_name;
};

// NamespaceComparison: resultado da comparação entre dois processos
struct NamespaceComparison {
    // ID do primeiro processo comparado
    pid_t pid1;
    
    // ID do segundo processo comparado
    pid_t pid2;
    
    // Total de namespaces verificados
    int total_namespaces;
    
    // Quantidade de namespaces que compartilham 
    int shared_namespaces;
    
    // Quantidade de namespaces que diferem
    int different_namespaces;
    
    // Array listando QUAIS tipos diferem
    std::vector<NamespaceType> differences;
};

// NamespaceGroup: agrupa processos que compartilham um namespace
// Usado para relatórios sistêmicos
struct NamespaceGroup {
    // Tipo do namespace
    NamespaceType type;
    
    // Identificador único do namespace
    // Todos os processos neste grupo têm o mesmo inode
    ino_t inode;
    
    // Quantos processos usam este namespace
    int process_count;
    
    // Array com IDs dos processos que o compartilham
    // Usado para análise detalhada
    std::vector<pid_t> pids;
};



// Lista todos os 7 namespaces de um processo
// Lê os links simbólicos em /proc/[pid]/ns/ e extrai os inodes
std::optional<ProcessNamespaces> list_process_namespaces(pid_t pid);

// Encontra todos os processos que compartilham um namespace específico
// Itera /proc/[1..max_pid] procurando pelo mesmo inode
std::vector<pid_t> find_processes_in_namespace(NamespaceType ns_type, ino_t ns_inode);

// Compara namespaces de dois processos
// Verifica quais tipos compartilham (inode igual) e quais diferem
std::optional<NamespaceComparison> compare_namespaces(pid_t pid1, pid_t pid2);

// Gera um relatório completo do sistema em CSV ou JSON
// Scanneia todos os processos em /proc e agrupa por namespace+inode
// Útil para entender topologia de isolamento do sistema
bool generate_namespace_report(const std::string& output_file, const std::string& format);

// Converte enum NamespaceType para string
// Útil para exibição e logging
std::string namespace_type_to_string(NamespaceType type);

// Converte string para enum NamespaceType
// Inverso da função anterior
// Exemplo: "pid" -> NamespaceType::PID
std::optional<NamespaceType> string_to_namespace_type(const std::string& str);

// Imprime no console os namespaces de um processo de forma formatada
// Mostra nome, link, inode e contagem total
void print_process_namespaces(const ProcessNamespaces& proc_ns);

// Imprime no console a comparação entre dois processos de forma formatada
// Mostra quais compartilham e quais diferem
void print_namespace_comparison(const NamespaceComparison& comparison);

#endif