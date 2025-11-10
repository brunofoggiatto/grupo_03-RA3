#ifndef NAMESPACE_HPP
#define NAMESPACE_HPP

#include <sys/types.h>
#include <cstdint>
#include <string>
#include <vector>
#include <optional>

// Tipos de namespace que o Linux suporta
enum class NamespaceType {
    CGROUP,
    IPC,
    MNT,
    NET,
    PID,
    USER,
    UTS,
    COUNT
};

// Info de um namespace específico
struct NamespaceInfo {
    NamespaceType type;
    std::string name;
    ino_t inode;
    std::string link;
    bool exists = false;
};

// Todos os namespaces de um processo
struct ProcessNamespaces {
    pid_t pid;
    std::vector<NamespaceInfo> namespaces;
    int ns_count = 0;
    std::string process_name;
};

// Comparação entre namespaces de dois processos
struct NamespaceComparison {
    pid_t pid1;
    pid_t pid2;
    int total_namespaces = 0;
    int shared_namespaces = 0;
    int different_namespaces = 0;
    std::vector<NamespaceType> differences;
};

// Grupo de processos que compartilham um namespace
struct NamespaceGroup {
    NamespaceType type;
    ino_t inode;
    int process_count = 0;
    std::vector<pid_t> pids;
};

// Funções principais

// Lista namespaces de um processo (le /proc/[pid]/ns/)
std::optional<ProcessNamespaces> list_process_namespaces(pid_t pid);

// Acha processos que tão no mesmo namespace
std::vector<pid_t> find_processes_in_namespace(NamespaceType ns_type, ino_t ns_inode);

// Compara namespaces entre dois processos
std::optional<NamespaceComparison> compare_namespaces(pid_t pid1, pid_t pid2);

// Gera relatório completo do sistema (CSV ou JSON)
bool generate_namespace_report(const std::string& output_file, const std::string& format);

// Funções auxiliares
std::string namespace_type_to_string(NamespaceType type);
std::optional<NamespaceType> string_to_namespace_type(const std::string& str);
void print_process_namespaces(const ProcessNamespaces& proc_ns);
void print_namespace_comparison(const NamespaceComparison& comparison);

#endif
