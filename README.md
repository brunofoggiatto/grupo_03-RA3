# Resource-Monitor RA3

**Sistema de Monitoramento, Análise e Controle de Recursos em Linux**

Integrantes:
- **Luiz Afonso Barbosa Silva** (Aluno 1) - Integração e main.cpp
- **Enzo Erkmann** (Aluno 2) - I/O Monitor e Testes
- **Bruno Henrique Foggiatto** (Aluno 3) - Namespace Analyzer
- **João Paulo Arantes Martins** (Aluno 4) - CGroup Manager

---

## Visão Geral

O `resource-monitor` é um conjunto de ferramentas integradas para explorar primitivas do kernel Linux (`namespaces`, `cgroups`, interfaces `/proc` e `/sys`):

- **Componente 1: Resource Profiler** – Monitora CPU, memória, I/O e rede de processos específicos
- **Componente 2: Namespace Analyzer** – Analisa e compara isolamento de processos
- **Componente 3: Control Group Manager** – Aplica limites de recursos e coleta métricas de cgroups

Ideal para estudar containerização, infraestrutura, ou entender como o kernel gerencia recursos.

---

## Principais Melhorias (Tarefa de Correção)

### CPU Monitor - Normalização por Cores
- CPU% agora é **normalizado pelo número de núcleos do sistema**
- Em sistema 4-core, processo usando 100% de 1 core = 25% (não 400%)
- Usa `std::thread::hardware_concurrency()` para detecção automática

### Memory Monitor - Tratamento de Erros Semânticos
- Retorna códigos: `ERR_PROCESS_NOT_FOUND (-2)`, `ERR_PERMISSION_DENIED (-3)`, `ERR_UNKNOWN (-4)`
- Mensagens de erro descritivas com `strerror(errno)`
- Validação de acesso antes de iniciar monitoramento

### I/O Monitor - Tratamento de Erros Idêntico
- Mesmo padrão de códigos semânticos que CPU e Memory
- Erros específicos por arquivo (`/proc/[pid]/io`, `/proc/net/tcp`)

### Monitor.hpp - Simplificação e Clareza
- Headers unificados com definição clara de estruturas e funções
- Definições de códigos de erro no topo
- Suporte a múltiplos overloads de `get_network_usage()`

---

## Requisitos e Dependências

| Recurso | Versão Mínima | Testado Em |
|---------|---------------|-----------|
| **SO** | Ubuntu 20.04+ | Ubuntu 24.04 LTS |
| **Kernel** | Linux 5.x+ | Linux 6.14.0-27-generic |
| **Compilador** | GCC 11+ | GCC 13.3.0 com C++23 |
| **CGroup** | v1 ou v2 | v2 (recomendado) |
| **Bibliotecas** | libc padrão | Sem dependências externas |

### Verificar Ambiente

```bash
# Kernel version
uname -r

# CGroup version
stat /sys/fs/cgroup/cgroup.controllers && echo "CGroup v2" || echo "CGroup v1"

# Compilador
g++ --version | grep -oP '\d+\.\d+\.\d+'

# Namespace support
ls /proc/$$/ns/ | wc -l  # Deve mostrar 7
```

---

##  Compilação

### Build Completo

```bash
make all
```

Gera:
- **Executável principal:** `bin/resource-monitor`
- **Testes:** `bin/test_cpu`, `bin/test_memory`, `bin/test_io`, `bin/test_cgroup`
- **Experimentos:** `bin/experimento{1..5}_*`

### Build Específico (Desenvolvimento)

```bash
# CPU Monitor apenas
g++ -std=c++23 -Wall -Wextra -O2 -I./include \
    -c src/cpu_monitor.cpp -o build/cpu_monitor.o

# Com linking final
g++ -std=c++23 -Wall -Wextra -O2 -I./include \
    src/cpu_monitor.cpp src/main.cpp -o bin/monitor -lm
```

### Limpeza

```bash
make clean      # Remove compilados
```

---

##  Instruções de Uso

### Interface Interativa

```bash
./bin/resource-monitor
```

Abre menu principal com 4 opções:

```
╔════════════════════════════════════════╗
║   SISTEMA DE MONITORAMENTO - RA3      ║
║   Recursos, Namespaces e CGroups      ║
╚════════════════════════════════════════╝

1. Resource Profiler (Componente 1)
2. Namespace Analyzer (Componente 2)
3. Control Group Manager (Componente 3)
4. Menu de Experimentos (1-5)
0. Sair

Escolha uma opção:
```

---

##  Componente 1: Resource Profiler

Monitora CPU, memória, I/O e rede de **um processo específico** em tempo real.

### Uso Interativo

```bash
./bin/resource-monitor
# Menu > 1
# PID: [número do processo]
# Duração (segundos): [padrão 30]
# Intervalo (segundos): [padrão 2]
```

### Exemplo Prático

```bash
# Encontrar PID do Firefox
$ pgrep -f firefox
12345

# Monitorar por 60 segundos, amostra a cada 1 segundo
./bin/resource-monitor
# Menu > 1
# PID: 12345
# Duração: 60
# Intervalo: 1

# Output em tempo real:
[2025-11-17 14:23:45] CPU:   2.50% | RSS:  256MB | IO:    1.2MB/s | TCP: 8
[2025-11-17 14:23:46] CPU:   2.75% | RSS:  257MB | IO:    0.9MB/s | TCP: 8
...
```

### Arquivo CSV Gerado

`monitoring_pid_12345.csv`:
```
timestamp,pid,cpu_percent,memory_rss_bytes,memory_vsz_bytes,memory_swap_bytes,...
2025-11-17 14:23:45,12345,2.50,268435456,536870912,0,...
2025-11-17 14:23:46,12345,2.75,269484032,536870912,0,...
```

### Colunas do CSV

| Coluna | Unidade | Descrição |
|--------|---------|-----------|
| `cpu_percent` | % | Uso normalizado pelo número de cores |
| `memory_rss_bytes` | bytes | Resident Set Size (memória física) |
| `memory_vsz_bytes` | bytes | Virtual Set Size (endereçamento) |
| `io_read_bytes` | bytes | Total de bytes lidos do disco |
| `io_write_bytes` | bytes | Total de bytes escritos no disco |
| `io_read_rate_bps` | B/s | Taxa de leitura de disco |
| `io_write_rate_bps` | B/s | Taxa de escrita de disco |
| `threads` | N | Número de threads ativas |
| `tcp_connections` | N | Conexões TCP abertas |

### Tratamento de Erros

```bash
# Processo não encontrado
$ ./bin/resource-monitor
# Menu > 1
# PID: 99999
# ERRO: Processo com PID 99999 não existe

# Sem permissão para ler outro usuário
# PID: 1
# ERRO: Permissão negada para acessar processo 1
# (Execute como root: sudo ./bin/resource-monitor)
```

---

##  Componente 2: Namespace Analyzer

Analisa e compara **isolamento de processos** usando namespaces do kernel.

### Tipos de Namespaces Suportados

| Tipo | Sigla | Função |
|------|-------|--------|
| **PID Namespace** | PID | Isola PIDs (tree de processos) |
| **Network Namespace** | NET | Isola stack de rede (interfaces, rotas) |
| **Mount Namespace** | MNT | Isola filesystems (/proc, /sys) |
| **IPC Namespace** | IPC | Isola filas de mensagens, semáforos |
| **UTS Namespace** | UTS | Isola hostname, domainname |
| **User Namespace** | USER | Isola UIDs/GIDs |
| **CGroup Namespace** | CGROUP | Isola view de cgroups |

### Uso 1: Listar Namespaces de um Processo

```bash
./bin/resource-monitor
# Menu > 2 > 1
# PID: 1 (init)

# Output:
Namespaces do processo PID 1 (systemd):
----------------------------------------
  cgroup:  cgroup:[4026531835] (inode: 4026531835)
  ipc:     ipc:[4026531839] (inode: 4026531839)
  mnt:     mnt:[4026531840] (inode: 4026531840)
  net:     net:[4026531956] (inode: 4026531956)
  pid:     pid:[4026531836] (inode: 4026531836)
  user:    user:[4026531837] (inode: 4026531837)
  uts:     uts:[4026531838] (inode: 4026531838)
Total: 7 namespaces
```

### Uso 2: Comparar Dois Processos

```bash
./bin/resource-monitor
# Menu > 2 > 2
# PID 1: 1 (systemd)
# PID 2: 1234 (bash)

# Output:
Comparacao entre PID 1 e PID 1234:
----------------------------------------
Total de namespaces verificados: 7
Namespaces compartilhados: 5
Namespaces diferentes: 2

Namespaces que diferem:
  - pid
  - net
```

### Uso 3: Gerar Relatório do Sistema

```bash
./bin/resource-monitor
# Menu > 2 > 3 (CSV) ou 4 (JSON)

# Gera: namespace_report.csv
Type,Inode,ProcessCount,PIDs
pid,4026531836,150
net,4026531956,140
mnt,4026531840,145
...
```

---

##  Componente 3: Control Group Manager

Cria, configura e monitora **limites de recursos** via cgroups v2.

 **Requer root:** `sudo ./bin/resource-monitor`

### Uso 1: Throttling de CPU

```bash
sudo ./bin/resource-monitor
# Menu > 3 > 1

# Output:
=== EXPERIMENTO 3: THROTTLING DE CPU ===
Criando cgroup experimental...
Testando limites de CPU:
----------------------------------------------------
Limite (cores)  |     CPU Usage (ns)     |    Status
----------------------------------------------------
        0.25    |        25.50           |    OK
        0.50    |        50.25           |    OK
        1.00    |       100.10           |    OK
        2.00    |       200.30           |    OK
```

### Uso 2: Limite de Memória

```bash
sudo ./bin/resource-monitor
# Menu > 3 > 2

# Output:
=== EXPERIMENTO 4: LIMITAÇÃO DE MEMÓRIA ===
Criando cgroup experimental...
Aplicando limite de 100MB...

Limites Aplicados:
  Limite configurado:       100 MB
  Uso atual:                 42 MB
  PIDs atuais:               1
```

### Uso 3: Limite de I/O

```bash
sudo ./bin/resource-monitor
# Menu > 3 > 3

# Executa benchmarks de I/O com throttling
# Gera: experimento5_results.csv
```

---

##  Experimentos (1-5)

Todos os experimentos documentados em `docs/experiments/`.

### Experimento 1: Overhead de Monitoramento

```bash
sudo ./bin/experimento1_overhead_monitoring
```

**Objetivo:** Medir o impacto do Resource Profiler no sistema.

**Métrica:** CPU overhead em diferentes intervalos de amostragem.

**Resultado:** Intervalo ótimo = **50-100ms** (~13% overhead)

### Experimento 2: Isolamento via Namespaces

```bash
./bin/experimento2_benchmark_namespaces
```

**Objetivo:** Medir overhead de criação de namespaces.

**Resultado:** Ranking de performance:
1. UTS: 18.19 µs
2. CGROUP: 18.53 µs
3. NET: 1652.62 µs (90x mais lento)

### Experimento 3: Throttling de CPU

```bash
sudo ./bin/experimento3_throttling_cpu
```

**Objetivo:** Validar precisão de limites de CPU.

**Resultado:** Precisão 99.5% (limite 1MB/s = 0.995 MB/s medido)

### Experimento 4: Limitação de Memória

```bash
sudo ./bin/experimento4_limitacao_memoria
```

**Objetivo:** Testar comportamento ao atingir limite.

**Resultado:** 100% de precisão, 2733 falhas de alocação contabilizadas

### Experimento 5: Limitação de I/O

```bash
sudo ./bin/experimento5_limitacao_io
```

**Objetivo:** Medir precisão de throttling de I/O.

**Resultado:** Baseline 544ms → Limite 1MB/s = 100.490ms

---

##  Tratamento de Erros (Melhorias na Tarefa)

### Códigos de Erro Semânticos

```cpp
#define ERR_PROCESS_NOT_FOUND  -2  // PID não existe
#define ERR_PERMISSION_DENIED  -3  // Sem permissão
#define ERR_UNKNOWN            -4  // Outro erro
```

Todos os monitors retornam um destes códigos + mensagem descritiva.

### Validação antes de Monitorar

```cpp
validateProcessAccess(pid):
  ✓ Verifica se PID ≤ 0 (inválido)
  ✓ Verifica se /proc/[pid]/ existe
  ✓ Tenta abrir /proc/[pid]/stat (valida permissão)
  → Lança exception descritiva se falhar
```

### Recuperação de Erros Durante Monitoramento

```
Máximo de 3 erros consecutivos:
  1 erro: Log e continua
  2 erros: Log e continua
  3 erros: Para monitoramento
  
Processo termina: Para imediatamente
```

---

##  Normalização de Métricas

### CPU% Normalizado por Cores

**Antes:**
```
Sistema 4-core, processo em 1 core a 100%:
CPU% = (delta_ticks / ticks_per_sec / interval) * 100 = 400%
```

**Depois:**
```
CPU% = 400% / 4 cores = 100% (interpretação correta)
```

Usa `std::thread::hardware_concurrency()` para detecção automática.

### Memória em Unidades Consistentes

- **Leitura:** VmRSS em KB de `/proc/[pid]/status`
- **Armazenamento:** Mantém em KB
- **Export CSV:** Converte para bytes (KB × 1024)
- **Display:** Mostra em MB (bytes / 1024 / 1024)

### I/O e Rede em Taxa

```
rate (B/s) = (current_bytes - prev_bytes) / interval_sec

Garante não-negatividade: if (rate < 0) rate = 0
```

---

##  Testes Unitários

```bash
# Teste de CPU Monitor
./bin/test_cpu

# Teste de Memory Monitor
./bin/test_memory

# Teste de I/O Monitor
./bin/test_io

# Teste do CGroup Manager (com e sem root)
./bin/test_cgroup

# Com root (todos os testes):
sudo ./bin/test_cgroup
```

---

##  Autores e Contribuições

### Aluno 1 - Luiz Afonso Barbosa Silva
- **Componente:** Resource Profiler (CPU + Memória) + Melhorias
- **Arquivos (Tarefa de Correção):**
  - `src/cpu_monitor.cpp` - Normalização por cores 
  - `src/memory_monitor.cpp` - Tratamento de erros 
  - `include/monitor.hpp` - Headers simplificados 
- **Testes:** `tests/test_cpu.cpp`, `tests/test_memory.cpp`
- **Experimento 1:** `tests/experimento1_overhead_monitoring.cpp`

### Aluno 2 - Enzo Erkmann
- **Componente:** Resource Profiler (I/O + Rede)
- **Arquivos:**
  - `src/io_monitor.cpp` (180 linhas) - Leitura de I/O por processo
  - `src/io_monitor.cpp` - Funções de rede do sistema e por PID
- **Testes:** `tests/test_io.cpp`, `tests/experimento5_limitacao_io.cpp`
- **Documentação:** `docs/experiments/experimento5_limitacao_io.md`

### Aluno 3 - Bruno Henrique Foggiatto
- **Componente:** Namespace Analyzer
- **Arquivos:**
  - `src/namespace_analyzer.cpp` (290 linhas) - Análise de isolamento
  - `include/namespace.hpp` (80 linhas) - Headers
- **Testes:**
  - `tests/experimento2_test_namespaces.cpp` (100 linhas)
  - `tests/experimento2_benchmark_namespaces.cpp` (300 linhas)
- **Documentação:** `docs/experiments/experimento2_namespaces.md`

### Aluno 4 - João Paulo Arantes Martins
- **Componente:** Control Group Manager + Integração
- **Arquivos:**
  - `src/cgroup_manager.cpp` (400+ linhas) - CGroup v1/v2
  - `include/cgroup_manager.hpp` (85 linhas) - Headers
  - `src/main.cpp` (350+ linhas) - Interface principal
- **Experimentos:**
  - `tests/experimento3_throttling_cpu.cpp`
  - `tests/experimento4_limitacao_memoria.cpp`
- **Documentação:**
  - `docs/experiments/experimento3_Throttling_de_CPU.md`
  - `docs/experiments/experimento4_Limitação_de_Memória.md`

---

##  Troubleshooting

### "Permissão negada" ao criar cgroup

```bash
# Solução: Execute com sudo
sudo ./bin/resource-monitor
# Menu > 3
```

### "Processo com PID X não existe"

```bash
# Solução: Verificar PID válido
ps aux | grep [nome_do_programa]
# Usar o PID listado
```

### "CGroup v2 não detectado"

```bash
# Verificar suporte
mount | grep cgroup2

# Se não estiver montado:
sudo mount -t cgroup2 none /sys/fs/cgroup
```

### Warnings na compilação

```
warning: unused parameter 'x' [-Wunused-parameter]
```

Solução: Use `(void)x;` ou `[[maybe_unused]]` antes da variável.

---

##  Referências e Documentação

### Documentação Oficial
- **Kernel Linux:** `/usr/src/linux/Documentation/cgroup-v*/`
- **Man pages:** `man 7 namespaces`, `man 7 cgroups`, `man proc`
- **LWN.net:** Artigos sobre namespaces e cgroups

### Literatura Científica
- **Felter et al. (2015):** Performance Evaluation of Virtual Machines and Linux Containers
- **Xavier et al. (2013):** Performance Evaluation of Container-based Virtualization
- **Soltesz et al. (2007):** Container-based OS Virtualization

### Ferramentas Relacionadas
- `htop` – Monitoramento de processos
- `iotop` – Monitoramento de I/O
- `systemd-cgtop` – Monitoramento de cgroups
- `docker stats` – Comparação com ferramentas existentes

---

##  Licença e Histórico

**Última atualização:** Novembro 2025

**Versão:** 2.0 (com melhorias de tratamento de erros e normalização de CPU%)

**Status:** Completo com todos os 5 experimentos documentados

---

##  Como Reproduzir os Experimentos

1. **Compilar tudo:**
   ```bash
   make all
   ```

2. **Executar cada experimento:**
   ```bash
   # Exp 1 (requer sudo)
   sudo ./bin/experimento1_overhead_monitoring
   
   # Exp 2
   ./bin/experimento2_benchmark_namespaces
   
   # Exp 3, 4, 5 (requerem sudo)
   sudo ./bin/resource-monitor  # Menu > 3
   ```

3. **Visualizar resultados:**
   ```bash
   cat experimento*_results.csv
   python3 scripts/visualize_namespaces.py experimento2_benchmark_results.csv
   ```

4. **Gerar relatórios:**
   ```bash
   # Gera ARCHITECTURE.md (este arquivo) com detalhes técnicos
   # Gera README.md (instruções de uso)
   ```

---

**Desenvolvido como trabalho avaliativo RA3 para Sistemas Operacionais e Infraestrutura de TI. Todos os componentes estão integrados e totalmente funcionais! **