# ARCHITECTURE.md

## 1. Visão geral

O projeto **resource-monitor** é um conjunto de três ferramentas de linha de comando que trabalham em cima das interfaces do kernel Linux:

- `/proc` – métricas de processos (CPU, memória, I/O, rede).
- `/proc/net/dev` – métricas agregadas de rede.
- `/sys/fs/cgroup` – métricas e configuração de cgroups (CPU, memória, blkio).
- syscalls / `/proc/*/ns/*` – análise de namespaces.

Os três binários principais são:

- `resource-monitor` → **Resource Profiler**
- `namespace-analyzer` → **Namespace Analyzer**
- `cgroup-manager` → **Control Group Manager**

Eles compartilham tipos e funções comuns via headers em `include/`.

---

## 2. Organização do código

Estrutura lógica do repositório (resumida):

```text
resource-monitor/
├── README.md  # Arquivo inicial que contextualiza o trabalho e dá instruções de uso
├── Makefile   # Arquivo responsável por compilar e construir o programa.docs/
├── docs/
│ ├── ARCHITECTURE.md          # Visão geral da arquitetura
│ └── experiments/             # Metodologias e análises dos experimentos
│     ├── experimento1.md
│     ├── experimento2.md
│     ├── experimento3.md
│     ├── experimento4.md
│     └── experimento5.md
├── include/
│   ├── monitor.hpp      # Tipos e funções comuns do Resource Profiler
│   ├── namespace.hpp    # Assinaturas do Namespace Analyzer
│   ├── cgroup.hpp       # Assinaturas do Control Group Manager
├── src/
│   ├── main.cpp             # CLI principal / integração
│   ├── cpu_monitor.cpp      # Coleta de métricas de CPU
│   ├── memory_monitor.cpp   # Coleta de métricas de memória
│   ├── io_monitor.cpp       # Coleta de I/O e rede
│   ├── namespace_analyzer.cpp # Análise e isolamento de namespaces
│   └── cgroup_manager.cppc    #Criação e visualização de cgroups
├── tests/
│   ├── test_cpu.cpp
│   ├── test_memory.cpp
│   ├── test_io.cpp
    ├── experimento1.cpp
    ├── experimento2.cpp
    ├── experimento3.cpp
    ├── experimento4.cpp
    ├── experimento5.cpp
