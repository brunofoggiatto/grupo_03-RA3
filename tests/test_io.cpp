/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: test_io.cpp
 *
 * PROPÓSITO:
 * Este arquivo é um programa de workload (carga de trabalho) "I/O-intensive".
 * É usado no Experimento 5 para testar a eficácia da limitação de I/O
 * (throttling) dos cgroups.
 *
 * RESPONSABILIDADE:
 * Aluno 2 (conforme divisão de tarefas).
 *
 * LÓGICA DE FUNCIONAMENTO:
 * 1. Escreve 100MB de dados em um arquivo temporário.
 * 2. Força a escrita desses dados da RAM para o disco (com sync()).
 * 3. Lê os 100MB de volta do disco.
 * 4. Mede o tempo total dessas operações e o imprime em milissegundos.
 *
 * COMO COMPILAR (Exemplo):
 * g++ -std=c++23 -Wall -Wextra -O2 -o test_io test_io.cpp
 * -----------------------------------------------------------------------------
 */

#include <fstream>    // Para std::ofstream (escrita) e std::ifstream (leitura)
#include <iostream>   // Para std::cout (impressão no console)
#include <vector>     // Para std::vector (buffer de dados)
#include <unistd.h>   // Para a syscall sync()
#include <chrono>     // Para medição de tempo (steady_clock)

int main() {
    std::cout << "Iniciando teste de I/O..." << std::endl;

    // Configurações do teste
    const size_t size = 100 * 1024 * 1024; // 100 MB de dados
    std::vector<char> data(1024, 'X');     // Buffer de 1KB para escrever

    // Inicia o cronômetro ANTES da operação de escrita
    auto start = std::chrono::steady_clock::now();

    // --- 1. OPERAÇÃO DE ESCRITA ---
    // Cria um arquivo temporário e escreve 100MB de dados nele
    std::ofstream out("teste_io.tmp", std::ios::binary);
    for (size_t i = 0; i < size / data.size(); ++i) {
        out.write(data.data(), data.size());
    }
    out.close();

    /*
     * --- 2. SINCRONIZAÇÃO (PONTO CRÍTICO DO EXPERIMENTO) ---
     *
     * Esta é a parte mais importante do teste.
     * Por padrão, o Linux usa o "page cache" (RAM) para I/O. A chamada
     * 'out.close()' NÃO garante que os dados foram para o disco.
     *
     * A syscall 'sync()' FORÇA o Kernel a "esvaziar" o cache e escrever
     * fisicamente os 100MB no disco. É isso que permite ao cgroup
     * (que limita o DISCO) "enxergar" e "estrangular" nosso programa.
     *
     * Sem esta linha, o teste rodaria na RAM e o limite do cgroup
     * seria ignorado.
     */
    sync();

    // --- 3. OPERAÇÃO DE LEITURA ---
    // Agora, lemos os dados de volta do disco para completar o workload.
    std::ifstream in("teste_io.tmp", std::ios::binary);
    char buffer[4096]; // Buffer de leitura
    while (in.read(buffer, sizeof(buffer))) {
        /* O loop continua enquanto a leitura for bem-sucedida */
    }
    in.close();

    // Para o cronômetro APÓS a leitura
    auto end = std::chrono::steady_clock::now();

    // --- 4. RESULTADOS ---
    // Calcula a duração total (Escrita + Sync + Leitura)
    // e imprime em milissegundos.
    std::cout << "Tempo total: "
              << std::chrono::duration_cast<std::chrono::milliseconds>(end - start).count()
              << " milissegundos.\n";
    std::cout << "Teste de I/O concluído." << std::endl;

    // --- 5. LIMPEZA ---
    // Remove o arquivo temporário
    std::remove("teste_io.tmp");
    
    return 0;
}