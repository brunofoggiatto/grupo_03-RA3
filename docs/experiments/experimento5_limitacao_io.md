# Experimento 5: Limitação de I/O

**Aluno:** Aluno 2

---

## Objetivo

Avaliar a precisão da limitação (throttling) de I/O (Bytes Por Segundo) utilizando *control groups* (cgroups).

## Ambiente

- **SO:** Ubuntu 24.04 LTS (em WSL2)
- **Kernel:** (Use `uname -r` para preencher. Ex: 6.14.0-27-generic)
- **CPU:** (Use `lscpu | grep "Model name"`. Ex: Intel Core i7-8565U)
- **Cgroup:** Versão 2 (v2), interface unificada
- **Disco Alvo (WSL2):** `8:48` (Disco virtual `sdd` do Linux, não o `/mnt/c`)

## Metodologia: O Desafio do Cache e Solução `sync()`

O *workload* (`tests/test_io.cpp`) escreve e lê 100MB de dados. Testes iniciais falharam em demonstrar a limitação (ex: 333ms *com* limite vs 466ms *sem* limite).

**Problema:** O Kernel Linux estava usando o *page cache* (na RAM) para as operações. O teste estava medindo apenas a velocidade da memória, não do disco. O *throttle* do cgroup (que limita o disco físico) não estava sendo ativado.

**Solução:** A `syscall sync()` (via `#include <unistd.h>`) foi adicionada ao código C++ imediatamente após a operação de escrita (`out.close();`). Esta chamada força o Kernel a "esvaziar" (sincronizar) o cache da RAM para o disco físico, garantindo que o limite do cgroup seja acionado.

## Resultados

As métricas reportadas abaixo atendem aos requisitos do PDF: "Impacto no tempo total de execução" e "Throughput medido vs limite configurado".

* **Workload:** Escrita de 100MB (com `sync()`) + Leitura de 100MB.
* **Disco Alvo:** `8:48`

| Limite Configurado (wbps) | Tempo Total (ms) | Tempo (s) | Throughput Teórico | Throughput Medido (100MB / Tempo) |
| :--- | :--- | :--- | :--- | :--- |
| **Sem Limite (Baseline)** | 544 ms | 0.54 s | N/A | ~183,8 MB/s |
| **1 MB/s** (1.048.576 B/s) | 100.490 ms | 100,49 s | 1,00 MB/s | **0,995 MB/s** |
| **5 MB/s** (5.242.880 B/s) | 20.332 ms | 20,33 s | 5,00 MB/s | **4,91 MB/s** |

## Análise

-   **Impacto no Tempo de Execução:** O impacto do *throttling* foi significativo e alinhado com a teoria. O *baseline* (sem limite) executou em 544ms. O limite de 1MB/s aumentou drasticamente o tempo para 100.490ms, e o de 5MB/s para 20.332ms.

-   **Throughput Medido vs. Configurado:** A precisão do cgroup v2 foi extremamente alta.
    -   No limite de 1MB/s, o *throughput* medido foi **0,995 MB/s** (99.5% de precisão).
    -   No limite de 5MB/s, o *throughput* medido foi **4,91 MB/s** (98.2% de precisão).

-   **Validação Teórica:** Os resultados validam os cálculos teóricos.
    -   *Teste 1 (1MB/s):* 100MB / 1MB/s = 100 segundos. (Resultado: 100,49s)
    -   *Teste 2 (5MB/s):* 100MB / 5MB/s = 20 segundos. (Resultado: 20,33s)

-   **Latência de I/O:** Esta métrica (o tempo de resposta de *uma* operação) não foi medida diretamente. O foco deste experimento foi o **throughput** e o **tempo total**, que são as métricas relevantes para avaliar a precisão do *throttling*.

## Como reproduzir

```bash
# 1. Compile o programa de benchmark do Experimento 5
# (O $(CXX) e $(CXXFLAGS) virão do Makefile principal)
make bin/experimento5_limitacao_io

# 2. Execute o benchmark automatizado com sudo
# (O programa precisa de sudo para manipular os cgroups)
sudo ./bin/experimento5_limitacao_io

# --- SAÍDA ESPERADA ---
# --- Detectando disco (MAJ:MIN) para o diretório atual... ---
# Disco alvo detectado: 8:1 (Device: /dev/sda1) <-- (Este valor será automático!)
#
# --- Executando Baseline (Sem Limite)... ---
# Resultado: 544 ms
#
# --- Executando Teste com Limite de 1 MB/s... ---
# Resultado: 100490 ms
#
# --- Executando Teste com Limite de 5 MB/s... ---
# Resultado: 20332 ms
#
# Resultados salvos em 'experimento5_results.csv'