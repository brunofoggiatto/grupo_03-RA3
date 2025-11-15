/*
 * -----------------------------------------------------------------------------
 * ARQUIVO: test_cpu.cpp
 *
 * PROPÓSITO:
 * Este arquivo é um programa de workload (carga de trabalho) "CPU-intensive".
 * É usado nos Experimentos 1 e 3 para testar:
 * 1. O overhead do monitor (Experimento 1).
 * 2. A eficácia da limitação de CPU (throttling) dos cgroups (Experimento 3).
 *
 * RESPONSABILIDADE:
 * Aluno 2 (conforme divisão de tarefas).
 *
 * LÓGICA DE FUNCIONAMENTO:
 * 1. Inicia um loop infinito de cálculos matemáticos pesados.
 * 2. O loop é projetado para tentar usar 100% de um único núcleo de CPU.
 * 3. O programa termina automaticamente após 10 segundos.
 *
 * COMO COMPILAR (Exemplo):
 * g++ -std=c++23 -Wall -Wextra -O2 -o test_cpu test_cpu.cpp -lm
 * -----------------------------------------------------------------------------
 */

#include <cmath>      // Para std::sin e std::cos (operações matemáticas pesadas)
#include <chrono>     // Para std::chrono (controlar a duração do teste)
#include <thread>     // Para std::this_thread (não usado aqui, mas relacionado)
#include <iostream>   // Para std::cout (impressão no console)

int main() {
    std::cout << "Iniciando teste de CPU (10 segundos)..." << std::endl;

    // Pega o tempo de início para controlar a duração total
    auto start = std::chrono::steady_clock::now();

    /*
     * -----------------------------------------------------------------------------
     * LOOP DE CARGA (CPU-INTENSIVE)
     *
     * Este loop roda enquanto o tempo total for menor que 10 segundos.
     *
     * COMO FUNCIONA:
     * O loop interno (for) executa 100.000 cálculos de seno e cosseno.
     * Essas são operações de ponto flutuante "caras" que exigem muito
     * da CPU. Elas também evitam que o compilador otimize o loop
     * (o compilador não pode prever o resultado, então ele *tem* que rodar).
     *
     * A variável 'x' é marcada como 'volatile' (implícito, mas boa prática)
     * para garantir que o cálculo seja realmente executado.
     * -----------------------------------------------------------------------------
     */
    while (std::chrono::steady_clock::now() - start < std::chrono::seconds(10)) {
        // A variável 'x' armazena o resultado para que o compilador
        // não otimize o loop interno e o remova.
        volatile double x = 0; 
        for (int i = 0; i < 100000; ++i) {
            x += std::sin(i) * std::cos(i);
        }
    }

    std::cout << "Teste de CPU finalizado." << std::endl;
    return 0;
}