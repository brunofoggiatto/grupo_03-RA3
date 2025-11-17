# ARCHITECTURE.md - Resource Monitor RA3

## 1. Visão Geral

O projeto **resource-monitor** é um conjunto de três ferramentas de linha de comando que exploram as interfaces do kernel Linux para monitoramento, análise e controle de recursos:

- `/proc` – métricas de processos (CPU, memória, I/O, rede)
- `/proc/net/dev` e `/proc/net/tcp` – métricas de rede do sistema e por processo
- `/sys/fs/cgroup` – métricas e configuração de cgroups (v1 e v2)
- `/proc/*/ns/*` – análise de namespaces do kernel

---

## 2. Arquitetura em Três Camadas

### Camada 1: Coleta de Métricas (Resource Profiler - Componente 1)

**Responsabilidade:** Ler dados brutos do `/proc` e `/sys` do Linux.

Cada monitor coleta dados específicos com tratamento robusto de erros:

#### 2.1 CPU Monitor (`cpu_monitor.cpp`)

- **Lê:** `/proc/[pid]/stat` e `/proc/[pid]/status`
- **Coleta:** 
  - `utime`, `stime` (tempo user e system em ticks)
  - `threads` (número de threads)
  - Context switches (voluntários e não-voluntários)
- **Normalização:** 
  - Converte ticks para segundos usando `sysconf(_SC_CLK_TCK)`
  - Normaliza CPU% pelo número de núcleos do sistema
  - Suporta sistemas multi-core corretamente
- **Tratamento de Erros:** Retorna códigos semânticos (ERR_PROCESS_NOT_FOUND, ERR_PERMISSION_DENIED, ERR_UNKNOWN)

#### 2.2 Memory Monitor (`memory_monitor.cpp`)

- **Lê:** `/proc/[pid]/status` e `/proc/meminfo`
- **Coleta:**
  - `VmRSS` (Resident Set Size em KB)
  - `VmSize` (Virtual Memory Size em KB)
  - `VmSwap` (Swap utilizado em KB)
  - Page faults (minor e major) de `/proc/[pid]/stat`
  - Calcula percentual de RAM em relação ao total do sistema
- **Tratamento de Erros:** Códigos semânticos consistentes com CPU monitor

#### 2.3 I/O Monitor (`io_monitor.cpp`)

- **Lê:** `/proc/[pid]/io` (por processo) e `/proc/net/dev` (sistema)
- **Coleta:**
  - `read_bytes`, `write_bytes` (I/O real de disco)
  - Taxa de I/O calculada em bytes/segundo
  - Rede do sistema: RX/TX total
  - Rede por processo: Conexões TCP ativas via `/proc/net/tcp` + `/proc/[pid]/fd/`
- **Tratamento de Erros:** Mesmo padrão semântico dos outros monitors

### Camada 2: Análise de Namespaces (Namespace Analyzer - Componente 2)

**Responsabilidade:** Inspecionar e comparar isolamento de processos.

Implementado em `namespace_analyzer.cpp` com funções principais:

- `list_process_namespaces(pid)` – Lista os 7 tipos de namespaces de um processo
- `find_processes_in_namespace(type, inode)` – Encontra processos que compartilham um namespace
- `compare_namespaces(pid1, pid2)` – Compara isolamento entre dois processos
- `generate_namespace_report(file, format)` – Gera relatórios CSV/JSON do sistema

**Namespaces Suportados:** CGROUP, IPC, MNT, NET, PID, USER, UTS

### Camada 3: Controle de Recursos (Control Group Manager - Componente 3)

**Responsabilidade:** Criar e gerenciar cgroups, aplicar limites de recursos.

Implementado em `cgroup_manager.cpp` com suporte a:

- **CGroup v2** (recomendado): Interface unificada em `/sys/fs/cgroup`
- **CGroup v1** (legado): Interface separada por controller

**Funcionalidades:**
- Criar/deletar cgroups
- Aplicar limites: CPU (quota/period ou cores), Memória (bytes/MB), I/O (BPS), PIDs
- Ler métricas: CPU usage, memory usage, PSI (Pressure Stall Information)
- Mover processos entre cgroups
- Gerar relatórios de utilização

---

## 3. Estrutura de Diretórios

```
resource-monitor/
├── README.md                           # Instruções de uso
├── Makefile                            # Build system
├── docs/
│   ├── ARCHITECTURE.md                # Este arquivo
│   └── experiments/                   # Documentação dos experimentos
│       ├── experimento1_overhead.md
│       ├── experimento2_namespaces.md
│       ├── experimento3_Throttling_de_CPU.md
│       ├── experimento4_Limitação_de_Memória.md
│       └── experimento5_limitacao_io.md
├── include/
│   ├── monitor.hpp                    # Headers do Resource Profiler
│   ├── namespace.hpp                  # Headers do Namespace Analyzer
│   └── cgroup.hpp                     # Headers do Control Group Manager
├── src/
│   ├── cpu_monitor.cpp                # Monitor de CPU (Aluno 1)
│   ├── memory_monitor.cpp             # Monitor de Memória (Aluno 1)
│   ├── io_monitor.cpp                 # Monitor de I/O + Rede (Aluno 2)
│   ├── namespace_analyzer.cpp         # Analyzer de Namespaces (Aluno 3)
│   ├── cgroup_manager.cpp             # Manager de CGroups (Aluno 4)
│   ├── main.cpp                       # Interface principal integrada (Aluno 4)
│   └── export_monitor.cpp             # Exportação CSV/JSON
├── tests/
│   ├── test_cpu.cpp                   # Workload CPU-intensive
│   ├── test_memory.cpp                # Workload Memory-intensive
│   ├── test_io.cpp                    # Workload I/O-intensive
│   ├── test_cgroup.cpp                # Testes unitários do CGroup Manager
│   ├── experimento1_overhead_monitoring.cpp
│   ├── experimento2_test_namespaces.cpp
│   ├── experimento2_benchmark_namespaces.cpp
│   ├── experimento3_throttling_cpu.cpp
│   ├── experimento4_limitacao_memoria.cpp
│   └── experimento5_limitacao_io.cpp
└── scripts/
    ├── visualize_namespaces.py        # Gera gráficos (opcional)
    └── compare_tools.sh               # Comparação com ferramentas existentes
```

---

## 4. Fluxo de Dados

### 4.1 Monitoramento de Processo (Resource Profiler)

```
Usuário solicita monitoramento de PID
        ↓
ResourceProfiler valida acesso ao processo
        ↓
Loop de monitoramento (intervalo configurável):
  ├─ get_cpu_usage(pid) → /proc/[pid]/stat, /proc/[pid]/status
  ├─ get_memory_usage(pid) → /proc/[pid]/status, /proc/meminfo
  ├─ get_io_usage(pid) → /proc/[pid]/io
  ├─ get_network_usage(pid) → /proc/net/tcp, /proc/[pid]/fd/
  │
  ├─ calculate_cpu_percent() → normaliza por cores
  ├─ calculate_io_rate() → diferença com leitura anterior
  ├─ calculate_network_rate() → idem I/O
  │
  └─ Salva em CSV: timestamp, pid, cpu%, mem, io, rede, ...
```

### 4.2 Análise de Namespaces

```
Usuário solicita análise de PID
        ↓
list_process_namespaces(pid)
  ├─ Lê /proc/[pid]/ns/cgroup → extrai inode
  ├─ Lê /proc/[pid]/ns/ipc → extrai inode
  ├─ Lê /proc/[pid]/ns/mnt → extrai inode
  ├─ Lê /proc/[pid]/ns/net → extrai inode
  ├─ Lê /proc/[pid]/ns/pid → extrai inode
  ├─ Lê /proc/[pid]/ns/user → extrai inode
  └─ Lê /proc/[pid]/ns/uts → extrai inode
        ↓
ProcessNamespaces { pid, processo_nome, array[7] }
```

**Comparação:** Compara inodes do mesmo tipo entre dois processos.

**Relatórios:** Scanneia `/proc/[1..max_pid]` e agrupa processos por namespace + inode.

### 4.3 Controle de Recursos (CGroup Manager)

```
Usuário solicita limite de CPU/Memória/I/O
        ↓
CGroupManager detecta versão (v1 ou v2)
        ↓
create_cgroup("/meu_cgroup")
  ├─ mkdir /sys/fs/cgroup/meu_cgroup
  └─ (CGroup v2: config subtree_control)
        ↓
set_cpu_limit(cores=0.5)
  ├─ CGroup v2: echo "50000 100000" → cpu.max
  └─ CGroup v1: echo "50000" → cpu.cfs_quota_us
        ↓
move_process_to_cgroup(pid)
  └─ echo pid → cgroup.procs (v2) ou tasks (v1)
        ↓
read_cpu_usage() / read_memory_usage()
  ├─ CGroup v2: cat cpu.stat, cat memory.current
  └─ CGroup v1: cat cpuacct.usage, cat memory.usage_in_bytes
```

---

## 5. Tratamento de Erros

Todos os monitors retornam códigos semânticos:

| Code | Significado | Ação Recomendada |
|------|-------------|------------------|
| 0 | Sucesso | Continuar |
| -2 (ERR_PROCESS_NOT_FOUND) | PID não existe | Parar monitoramento, relatar ao usuário |
| -3 (ERR_PERMISSION_DENIED) | Sem permissão para ler | Sugerir sudo ou mesma permissão |
| -4 (ERR_UNKNOWN) | Outro erro | Log com `strerror(errno)` |

O `main.cpp` captura esses retornos e:
- Valida permissões antes de iniciar monitoramento
- Trata processos que terminam durante monitoramento
- Implementa retry com limite máximo de erros consecutivos
- Exibe mensagens informativas ao usuário

---

## 6. Normalização de Métricas

### CPU% Normalizado por Cores

**Antes:**
```
Sistema 4-core, processo em 1 core a 100%:
CPU% = (delta_ticks / ticks_per_sec / interval) * 100 = 400%
```

**Depois:**
```
num_cores = std::thread::hardware_concurrency()
normalized_cpu% = cpu% / num_cores
Resultado: 100% em sistema 4-core (uso efetivo do 1 core = 100%)
```

### Memória em Unidades Consistentes

- **Entrada:** VmRSS, VmSize, VmSwap lidos em KB de `/proc`
- **Armazenamento:** Mantém em KB no `ProcStats`
- **Exportação CSV:** Converte para bytes (KB × 1024)
- **Display:** Converte para MB para legibilidade

### I/O e Rede em Taxa (B/s)

```
current_bytes = valor_lido_em_t
prev_bytes = valor_lido_em_t-1
interval = t - t-1 (segundos)

rate = (current_bytes - prev_bytes) / interval
```

Garante que a taxa é sempre não-negativa (descarta valores antigos se houver reset).

---

## 7. Sincronização Thread-Safe

### Timestamp com `localtime_r`

```cpp
time_t t = chrono::system_clock::to_time_t(time_now);
struct tm tm_buf;
localtime_r(&t, &tm_buf);  // Thread-safe (r = reentrant)
```

### Sinal SIGINT para Parar Gracioso

```cpp
atomic<bool> monitoring_active{true};

signal(SIGINT, signal_handler);

// Em signal_handler:
monitoring_active = false;  // Atomic, não precisa mutex
```

---

## 8. Integração dos Componentes

```
main.cpp (CLI)
    ├─ ResourceProfiler → CPU/Memory/IO Monitors
    ├─ Namespace Analyzer → namespace_analyzer.cpp
    └─ CGroupManager → cgroup_manager.cpp

Exportação:
    ├─ CSV: monitoring_pid_[PID].csv
    ├─ CSV: namespace_report.csv
    ├─ JSON: namespace_report.json
    └─ TXT: cgroup_report.txt
```

---

## 9. Configurações de Compilação

```makefile
CXXFLAGS = -std=c++23 -Wall -Wextra -O2 -I./include
LDFLAGS = -lm
```

- **C++23:** Suporta features recentes (std::optional, std::format no futuro)
- **-Wall -Wextra:** Detecta todas as warnings
- **-O2:** Otimização balanceada (performance vs debug info)
- **-lm:** Link com libm (math library) para cálculos de `thread::hardware_concurrency`

---

## 10. Requisitos do Sistema

| Requisito | Versão Mínima | Testado Em |
|-----------|---------------|-----------|
| **Kernel Linux** | 5.x+ | 6.14.0 |
| **C++ Compiler** | GCC 11+ | GCC 13.3.0 |
| **CGroup** | v1 ou v2 | v2 (Ubuntu 24.04+) |
| **Namespaces** | Kernel 3.8+ | Todos os 7 tipos |

---

## 11. Performance e Overhead

Medido no Experimento 1:

| Intervalo | CPU Overhead | Latência de Coleta |
|-----------|--------------|-------------------|
| 10ms | +31.41% | 559 µs |
| 50ms | +13.23% | 479 µs |
| 100ms | +12.48% | 424 µs |
| 500ms | +86.59% | 558 µs |

**Recomendação:** Use intervalo de **50-100ms** para balanço ideal.

---

## 12. Próximas Melhorias

- Suporte a dashboard web em tempo real (WebSocket)
- Detecção automática de anomalias (desvio padrão, IQR)
- Integração com Prometheus/Grafana
- Profiling de memória com malloc hooks
- Suporte a containers (Docker/Podman) via cgroup path