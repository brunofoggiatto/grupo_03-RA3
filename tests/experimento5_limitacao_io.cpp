/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: tests/experimento5_limitacao_io.cpp
 *
 * PROPÓSITO:
 * Programa de benchmark C++ que AUTOMATIZA o Experimento 5.
 * Resolve a violação do PDF (que proíbe scripts para tarefas principais).
 *
 * LÓGICA DE FUNCIONAMENTO:
 * 1. Verifica se está rodando como 'sudo' (root).
 * 2. DETECTA AUTOMATICAMENTE o disco (MAJ:MIN) onde está rodando.
 * 3. Cria um cgroup v2 ('exp5_io') e habilita o controlador 'io'.
 * 4. Roda o workload (escreve 100MB com sync()) 3 vezes:
 * - Sem limite (no cgroup raiz)
 * - Com limite de 1 MB/s
 * - Com limite de 5 MB/s
 * 5. Salva os resultados em 'experimento5_results.csv'.
 *
 * COMO COMPILAR:
 * g++ -std=c++23 -Wall -Wextra -O2 -o ./bin/experimento5_limitacao_io tests/experimento5_limitacao_io.cpp
 * -----------------------------------------------------------------------------
 */

#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <unistd.h>  // Para geteuid(), getpid(), sync()
#include <sys/stat.h> // Para mkdir()
#include <cstdio>    // Para popen, pclose, fgets (executar comandos)
#include <memory>    // Para std::unique_ptr
#include <array>     // Para std::array
#include <stdexcept> // Para std::runtime_error

/**
 * @brief Executa um comando shell e retorna sua saída como uma string.
 * @param cmd O comando a ser executado.
 * @return A saída (stdout) do comando.
 */
std::string exec(const char* cmd) {
    std::array<char, 128> buffer;
    std::string result;
    // Abre um pipe para ler a saída do comando
    std::unique_ptr<FILE, decltype(&pclose)> pipe(popen(cmd, "r"), pclose);
    if (!pipe) {
        throw std::runtime_error("popen() failed!");
    }
    // Lê a saída linha por linha
    while (fgets(buffer.data(), buffer.size(), pipe.get()) != nullptr) {
        result += buffer.data();
    }
    // Remove a quebra de linha no final, se houver
    if (!result.empty() && result[result.length()-1] == '\n') {
        result.pop_back();
    }
    return result;
}


class IOThrottleExperiment {
    const std::string CGROUP_ROOT = "/sys/fs/cgroup";
    const std::string CGROUP_NAME = "exp5_io";
    const std::string CGROUP_PATH = CGROUP_ROOT + "/" + CGROUP_NAME;
    std::string DEVICE; // NÃO é 'const', será detectada dinamicamente

    /**
     * @brief Detecta o dispositivo (MAJ:MIN) do diretório atual.
     * Roda 'findmnt' e 'lsblk' para achar o disco, tornando o
     * script portátil para qualquer máquina (incluindo o PC do professor).
     */
    bool find_device() {
        try {
            std::cout << "--- Detectando disco (MAJ:MIN) para o diretório atual... ---\n";
            // Acha a "fonte" (partição) onde o diretório atual (.) está
            std::string device_path = exec("findmnt -n -o SOURCE --target .");
            if (device_path.empty()) {
                std::cerr << "ERRO: findmnt não conseguiu achar o device para '.'\n";
                return false;
            }

            // Usa o lsblk para pegar os números MAJ:MIN daquele device
            std::string cmd = "lsblk -dno MAJ:MIN " + device_path;
            DEVICE = exec(cmd.c_str());
            
            if (DEVICE.empty()) {
                std::cerr << "ERRO: lsblk não conseguiu achar o MAJ:MIN para " << device_path << "\n";
                return false;
            }
            
            std::cout << "Disco alvo detectado: " << DEVICE << " (Device: " << device_path << ")\n";
            return true;
            
        } catch (const std::exception& e) {
            std::cerr << "Exceção ao detectar o disco: " << e.what() << '\n';
            return false;
        }
    }

    /**
     * @brief Cria o cgroup e habilita o controlador 'io'.
     */
    bool create_cgroup() {
        mkdir(CGROUP_PATH.c_str(), 0755);
        // Habilita o controlador 'io' no cgroup pai (raiz)
        // Usamos 'system' para 'echo' pois 'ofstream' sobrescreve o arquivo
        // e apagaria outros controladores (ex: +cpu, +memory).
        // O 2>/dev/null suprime erros (caso já esteja habilitado).
        system("echo '+io' > /sys/fs/cgroup/cgroup.subtree_control 2>/dev/null");
        return true;
    }

    /**
     * @brief Define o limite de escrita (wbps) no cgroup.
     */
    void set_limit(const std::string& bps) {
        std::ofstream io_max(CGROUP_PATH + "/io.max");
        io_max << DEVICE << " wbps=" << bps << "\n";
    }

    /**
     * @brief Move um PID para um cgroup específico.
     */
    void move_to_cgroup(pid_t pid, const std::string& group_path) {
        std::ofstream procs(group_path + "/cgroup.procs");
        procs << pid;
    }

    /**
     * @brief Roda o workload de 100MB de escrita com sync().
     * @return O tempo de execução em milissegundos.
     */
    long run_workload() {
        auto start = std::chrono::high_resolution_clock::now();

        // Escreve 100MB no diretório ATUAL
        std::ofstream out("./teste_io.tmp", std::ios::binary); 
        std::vector<char> buf(1024*1024, 'A'); // Buffer de 1MB
        for (int i = 0; i < 100; i++) { // Escreve 100x
            out.write(buf.data(), buf.size());
        }
        out.close();
        
        // CRÍTICO: Força o cache (RAM) a ser escrito no disco (onde o limite atua)
        sync(); 
        
        std::remove("./teste_io.tmp"); // Limpa o arquivo

        auto end = std::chrono::high_resolution_clock::now();
        return std::chrono::duration_cast<std::chrono::milliseconds>(end-start).count();
    }

public:
    void run() {
        if (!find_device()) {
            std::cerr << "Falha ao detectar o disco. Abortando.\n";
            return;
        }
        
        create_cgroup();
        pid_t my_pid = getpid();

        // --- Teste 1: Sem limite ---
        std::cout << "\n--- Executando Baseline (Sem Limite)... ---\n";
        // Move o processo para o cgroup raiz (sem limite)
        move_to_cgroup(my_pid, CGROUP_ROOT);
        long t1 = run_workload();
        std::cout << "Resultado: " << t1 << " ms\n";

        // Mover o processo para o cgroup com limite
        move_to_cgroup(my_pid, CGROUP_PATH);

        // --- Teste 2: 1 MB/s ---
        std::cout << "\n--- Executando Teste com Limite de 1 MB/s... ---\n";
        set_limit("1048576");
        long t2 = run_workload();
        std::cout << "Resultado: " << t2 << " ms\n";

        // --- Teste 3: 5 MB/s ---
        std::cout << "\n--- Executando Teste com Limite de 5 MB/s... ---\n";
        set_limit("5242880");
        long t3 = run_workload();
        std::cout << "Resultado: " << t3 << " ms\n";

        // Limpeza: move de volta para o raiz
        move_to_cgroup(my_pid, CGROUP_ROOT);

        // Exportar CSV
        std::ofstream csv("experimento5_results.csv");
        csv << "limite,tempo_ms,throughput_mbps\n";
        csv << "Sem limite," << t1 << "," << (100.0 / (t1 / 1000.0)) << "\n";
        csv << "1MB/s," << t2 << "," << (100.0 / (t2 / 1000.0)) << "\n";
        csv << "5MB/s," << t3 << "," << (100.0 / (t3 / 1000.0)) << "\n";
        std::cout << "\nResultados salvos em 'experimento5_results.csv'\n";
    }
};

int main() {
    if (geteuid() != 0) {
        std::cerr << "ERRO: Este script deve ser executado com sudo (root)!\n";
        return 1;
    }
    IOThrottleExperiment().run();
    return 0;
}