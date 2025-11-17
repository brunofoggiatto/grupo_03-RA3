#include <iostream>
#include <fstream>
#include <chrono>
#include <vector>
#include <thread>
#include <cstring>
#include <cstdlib>
#include <unistd.h>
#include <sys/stat.h>
#include <sys/types.h>
#include <cerrno>
#include <fcntl.h>
#include <sstream>
#include <dirent.h>

// Adicionar includes para major() e minor()
#include <sys/sysmacros.h>

class IOThrottleExperiment {
private:
    std::string CGROUP_PATH;
    std::string device_major_minor;

    // Detectar dispositivo de bloco automaticamente
    bool detect_block_device() {
        // Tentar detectar o dispositivo raiz
        std::ifstream mounts("/proc/mounts");
        std::string line;
        
        while (std::getline(mounts, line)) {
            if (line.find(" / ") != std::string::npos) {
                // Encontrou a partição raiz
                std::istringstream iss(line);
                std::string device, mountpoint, fstype;
                iss >> device >> mountpoint >> fstype;
                
                if (device.find("/dev/") == 0) {
                    // Obter major:minor do dispositivo - CORREÇÃO AQUI
                    struct stat st;
                    if (stat(device.c_str(), &st) == 0) {
                        int major_num = major(st.st_rdev);  // Renomeado para evitar conflito
                        int minor_num = minor(st.st_rdev);  // Renomeado para evitar conflito
                        device_major_minor = std::to_string(major_num) + ":" + std::to_string(minor_num);
                        std::cout << " Dispositivo detectado: " << device 
                                  << " (" << device_major_minor << ")" << std::endl;
                        return true;
                    }
                }
            }
        }
        
        // Fallback para valores comuns
        std::cout << "  Não foi possível detectar dispositivo, usando fallback..." << std::endl;
        device_major_minor = "8:0"; // /dev/sda comum
        return true;
    }

    // Restante do código mantido igual...
    bool is_cgroup_v2() {
        struct stat st;
        return stat("/sys/fs/cgroup/cgroup.controllers", &st) == 0;
    }

    bool create_cgroup() {
        if (is_cgroup_v2()) {
            CGROUP_PATH = "/sys/fs/cgroup/exp5_io";
        } else {
            CGROUP_PATH = "/sys/fs/cgroup/blkio/exp5_io";
        }

        // Criar diretório do cgroup
        if (mkdir(CGROUP_PATH.c_str(), 0755) != 0 && errno != EEXIST) {
            std::cerr << " Erro ao criar cgroup " << CGROUP_PATH 
                      << ": " << strerror(errno) << std::endl;
            return false;
        }

        // Configurações específicas para cgroup v2
        if (is_cgroup_v2()) {
            std::ofstream subtree_control("/sys/fs/cgroup/cgroup.subtree_control");
            if (subtree_control.is_open()) {
                subtree_control << "+io" << std::endl;
                subtree_control.close();
            }
        }

        std::cout << " CGroup criado: " << CGROUP_PATH << std::endl;
        return true;
    }

    void set_io_limit(const std::string& bps_limit) {
        if (is_cgroup_v2()) {
            // CGroup v2
            std::ofstream io_max(CGROUP_PATH + "/io.max");
            if (io_max.is_open()) {
                io_max << device_major_minor << " wbps=" << bps_limit << std::endl;
                io_max.close();
                std::cout << " Limite CGroup v2: " << bps_limit << " bytes/s" << std::endl;
            } else {
                std::cerr << " Não foi possível definir limite de I/O (io.max)" << std::endl;
            }
        } else {
            // CGroup v1
            std::ofstream blkio_limit(CGROUP_PATH + "/blkio.throttle.write_bps_device");
            if (blkio_limit.is_open()) {
                blkio_limit << device_major_minor << " " << bps_limit << std::endl;
                blkio_limit.close();
                std::cout << " Limite CGroup v1: " << bps_limit << " bytes/s" << std::endl;
            } else {
                std::cerr << " Não foi possível definir limite de I/O (blkio.throttle.write_bps_device)" << std::endl;
            }
        }
    }

    void move_process_to_cgroup() {
        std::string procs_file;
        
        if (is_cgroup_v2()) {
            procs_file = CGROUP_PATH + "/cgroup.procs";
        } else {
            procs_file = CGROUP_PATH + "/tasks";
        }

        std::ofstream procs(procs_file);
        if (procs.is_open()) {
            procs << getpid() << std::endl;
            procs.close();
            std::cout << " Processo movido para o cgroup" << std::endl;
        } else {
            std::cerr << " Erro ao mover processo para cgroup" << std::endl;
        }
    }

    void cleanup_cgroup() {
        // Mover processo de volta para root
        std::ofstream root_procs;
        if (is_cgroup_v2()) {
            root_procs.open("/sys/fs/cgroup/cgroup.procs");
        } else {
            root_procs.open("/sys/fs/cgroup/blkio/tasks");
        }
        
        if (root_procs.is_open()) {
            root_procs << getpid() << std::endl;
            root_procs.close();
        }

        // Remover cgroup
        if (rmdir(CGROUP_PATH.c_str()) == 0) {
            std::cout << " Cgroup removido: " << CGROUP_PATH << std::endl;
        } else if (errno != ENOENT) {
            std::cerr << "  Aviso: Não foi possível remover " << CGROUP_PATH 
                      << ": " << strerror(errno) << std::endl;
        }
    }

    long run_workload(int size_mb) {
        std::cout << "💾 Executando workload de " << size_mb << "MB..." << std::endl;
        
        auto start = std::chrono::high_resolution_clock::now();

        std::string filename = "/tmp/io_benchmark_" + std::to_string(getpid()) + ".bin";
        std::ofstream out(filename, std::ios::binary | std::ios::trunc);
        
        if (!out.is_open()) {
            std::cerr << " Erro ao criar arquivo: " << filename << std::endl;
            return -1;
        }

        // Buffer de 1MB preenchido com dados
        std::vector<char> buffer(1024 * 1024);
        for (size_t i = 0; i < buffer.size(); i++) {
            buffer[i] = static_cast<char>(i % 256);
        }

        bool success = true;
        for (int i = 0; i < size_mb && success; i++) {
            out.write(buffer.data(), buffer.size());
            if (!out) {
                std::cerr << " Erro na escrita no MB " << (i + 1) << std::endl;
                success = false;
            }
            
            // Flush periódico
            if ((i + 1) % 10 == 0) {
                out.flush();
            }
        }
        
        out.close();

        if (success) {
            // Sync e remover arquivo
            sync();
            std::remove(filename.c_str());
        } else {
            std::remove(filename.c_str());
            return -1;
        }

        auto end = std::chrono::high_resolution_clock::now();
        long duration = std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count();
        
        std::cout << " Duração: " << duration << " ms" << std::endl;
        return duration;
    }

public:
    void run() {
        std::cout << " INICIANDO EXPERIMENTO 5 - LIMITAÇÃO DE I/O" << std::endl;
        std::cout << "=============================================" << std::endl;

        // Detectar versão do CGroup
        if (is_cgroup_v2()) {
            std::cout << " Sistema usa: CGroup v2" << std::endl;
        } else {
            std::cout << " Sistema usa: CGroup v1" << std::endl;
        }

        // Detectar dispositivo
        if (!detect_block_device()) {
            std::cerr << " Não foi possível detectar dispositivo de bloco" << std::endl;
            return;
        }

        if (!create_cgroup()) {
            return;
        }

        // Configurações de teste
        struct TestCase {
            std::string name;
            std::string limit_bps;
            int file_size_mb;
        };

        std::vector<TestCase> test_cases = {
            {"Sem limite", "max", 50},
            {"1 MB/s", "1048576", 20},
            {"5 MB/s", "5242880", 20}
        };

        std::vector<long> results;

        for (const auto& test : test_cases) {
            std::cout << "\n TESTE: " << test.name << std::endl;
            std::cout << "---------------------------------------------" << std::endl;

            // Aplicar limite (exceto para "max")
            if (test.limit_bps != "max") {
                set_io_limit(test.limit_bps);
            }

            // Mover processo para cgroup
            move_process_to_cgroup();

            // Pequena pausa para estabilização
            std::this_thread::sleep_for(std::chrono::seconds(1));

            // Executar workload
            long duration_ms = run_workload(test.file_size_mb);
            
            if (duration_ms > 0) {
                double throughput_mbps = (test.file_size_mb * 1024.0 * 1024.0) / (duration_ms / 1000.0);
                throughput_mbps /= (1024.0 * 1024.0); // Converter para MB/s
                
                std::cout << " Throughput: " << throughput_mbps << " MB/s" << std::endl;
                results.push_back(duration_ms);
            } else {
                results.push_back(-1);
            }

            // Pausa entre testes
            std::this_thread::sleep_for(std::chrono::seconds(2));
        }

        // Gerar relatório CSV
        std::ofstream csv("experimento5_results.csv");
        csv << "limite,arquivo_mb,tempo_ms,throughput_mbps\n";
        
        for (size_t i = 0; i < test_cases.size(); i++) {
            if (results[i] > 0 && i < results.size()) {
                double throughput_mbps = (test_cases[i].file_size_mb * 1024.0 * 1024.0) / (results[i] / 1000.0);
                throughput_mbps /= (1024.0 * 1024.0);
                
                csv << test_cases[i].name << ","
                    << test_cases[i].file_size_mb << ","
                    << results[i] << ","
                    << throughput_mbps << "\n";
            }
        }
        csv.close();

        // Limpeza
        cleanup_cgroup();

        std::cout << "\n EXPERIMENTO 5 CONCLUÍDO!" << std::endl;
        std::cout << " Resultados salvos em: experimento5_results.csv" << std::endl;
        std::cout << " Dica: Execute 'cat experimento5_results.csv' para ver os resultados" << std::endl;
    }
};

int main() {
    if (geteuid() != 0) {
        std::cerr << " Este experimento requer privilégios de root!" << std::endl;
        std::cerr << "   Execute com: sudo ./bin/experimento5_limitacao_io" << std::endl;
        return 1;
    }

    std::cout << " Verificando ambiente CGroup..." << std::endl;

    // Verificar se CGroup está montado
    struct stat st;
    if (stat("/sys/fs/cgroup", &st) != 0) {
        std::cerr << " CGroup não está montado em /sys/fs/cgroup" << std::endl;
        return 1;
    }

    IOThrottleExperiment experiment;
    experiment.run();

    return 0;
}