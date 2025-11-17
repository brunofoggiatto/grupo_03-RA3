# Resource-monitor

Integrantes:
Luiz Afonso Barbosa Silva (Aluno A) 
Enzo Erkmann (Aluno B)
Bruno Hnenrique Foggiatto (Aluno C)
João Paulo Arantes Martins (Aluno D)


Conjunto de ferramentas para **monitoramento, análise e controle de recursos** de processos e containers em Linux.

O projeto explora primitivas do kernel (`namespaces`, `cgroups` e interfaces `/proc` e `/sys`) para:

- Coletar métricas detalhadas de processos.
- Analisar isolamento por namespaces.
- Aplicar limites de CPU, memória e I/O via cgroups.
- Permitir **experimentos reproduzíveis** de overhead de monitoramento e precisão de throttling.

---

## Visão geral

O `resource-monitor` foi pensado para uso em ambiente Linux  (Ubuntu 24.04+), focando em:

- Monitorar PIDs específicos em intervalos configuráveis.
- Estudar como o kernel isola processos usando namespaces.
- Controlar recursos via cgroups (v1 e, opcionalmente, v2).

---

## Objetivos

- **Métricas detalhadas por processo**
  - CPU: tempo user/system, número de threads, context switches voluntários e não voluntários.
  - Memória: RSS, VSZ, page faults (minor/major), uso de swap, `%` de RAM usada.
  - I/O: bytes lidos/escritos, syscalls, operações de disco.
  - Rede: bytes RX/TX, pacotes, conexões (quando implementado).

- **Namespaces**
  - Mapear e listar namespaces de um processo.
  - Comparar namespaces entre processos.
  - Medir overhead de criação de namespaces.

- **cgroups**
  - Ler métricas de controladores (CPU, memory, blkio, pids).
  - Criar cgroups experimentais e aplicar limites.
  - Mover processos entre cgroups.
  - Avaliar **precisão do throttling** (CPU, memória, I/O).

- **Exportação de dados**
  - Gerar **CSV/JSON** para gráficos, dashboards e análise científica.
  - Documentar experimentos em documentos

---

## Componentes principais

### 1. Resource Profiler

Ferramenta principal de monitoramento de processos.

- Monitora um ou mais PIDs em intervalo configurável.
- Calcula CPU% e taxas de I/O.
- Lê estatísticas em `/proc` e `/sys`.
- Exporta dados em **CSV** ou **JSON**.
- Trata erros comuns (PID inexistente, permissões insuficientes, etc.).

### 2. Namespace Analyzer

Ferramenta para inspecionar e comparar namespaces.

- Lista namespaces associados a um processo.
- Encontra processos que compartilham um namespace específico.
- Compara namespaces entre dois PIDs.
- Mede tempo de criação de namespaces para avaliar overhead.

### 3. Control Group Manager

Ferramenta para manipulação de cgroups.

- Lê métricas de:
  - `cpu`
  - `memory`
  - `blkio`
  - `pids`
- Cria cgroups experimentais para testes.
- Aplica limites de:
  - CPU (quota/period ou equivalente em “cores”).
  - Memória (limites em bytes/MB).
  - I/O (limites de BPS).
- Move processos entre cgroups e mede:
  - Precisão do throttling.
  - Impacto em throughput e latência.

---

## Requisitos e Dependências

- **Sistema Operacional**: Ubuntu 24.04 LTS ou superior
- **Kernel**: Linux 6.x+ com suporte a namespaces e cgroups
- **Compilador**: GCC 13+ com suporte a C++23
- **Ferramentas**: Make, sudo (para alguns experimentos)
- **Bibliotecas**: libc padrão (sem dependências externas)

---

## Instruções de Compilação

### Compilar todos os componentes
```bash
make all
```

### Namespace Analyzer (Aluno C)
```bash
# Testes funcionais
g++ -std=c++23 -Wall -Wextra -o experimento2_test_namespaces \
    tests/experimento2_test_namespaces.cpp src/namespace_analyzer.cpp

# Benchmark de overhead
g++ -std=c++23 -Wall -Wextra -o experimento2_benchmark_namespaces \
    tests/experimento2_benchmark_namespaces.cpp src/namespace_analyzer.cpp
```

### Limpar
```bash
make clean
```

---

## Instruções de Uso com Exemplos

### Namespace Analyzer

**Testes funcionais:**
```bash
./experimento2_test_namespaces
```

Saída: Lista namespaces do processo, encontra processos compartilhando namespaces, compara processos, gera relatórios CSV/JSON.

**Benchmark de overhead:**
```bash
sudo ./experimento2_benchmark_namespaces
```

Mede tempo de criação de cada tipo de namespace em 200 iterações. Gera `experimento2_benchmark_results.csv` com estatísticas.

---

## Experimentos Realizados

### Experimento 1: Overhead de Monitoramento
- **Objetivo**: Medir o impacto do próprio profiler no sistema
- **Metodologia**: Executar workload de referência com e sem monitoramento em diferentes intervalos
- **Métricas**: Tempo de execução, CPU overhead, latência de sampling
- **Documentação completa**: `docs/experiments/experimento1_overhead.md`

### Experimento 2: Isolamento via Namespaces (Aluno 3)
- **Objetivo**: Validar efetividade do isolamento e medir overhead de criação
- **Metodologia**: 200 iterações, medição apenas da syscall unshare()
- **Resultados**: UTS/CGROUP (~18µs), NET (~1652µs - 90x mais lento)
- **Documentação completa**: `docs/experiments/experimento2_namespaces.md`

---

## Guia de Uso Interativo

### Interface Principal

```bash
./bin/resource-monitor
```

Menu principal com 4 opções:

1. **Resource Profiler (Componente 1)** - Monitorar processos
2. **Namespace Analyzer (Componente 2)** - Analisar namespaces  
3. **Control Group Manager (Componente 3)** - Gerenciar cgroups (requer sudo)
4. **Menu de Experimentos** - Executar experimentos 1-5

### Componente 1: Resource Profiler

Monitora CPU, memória e I/O de um processo em tempo real:

```bash
./bin/resource-monitor
# Menu > 1
# PID: [número do processo]
# Duração (segundos): [padrão 30]
# Intervalo (segundos): [padrão 2]
```

**Saída:**
- Terminal: linhas em tempo real com `[timestamp] CPU: X% | RSS: YMB | IO: ZMB/s`
- Arquivo CSV: `monitoring_pid_[PID].csv` com colunas:
  - timestamp, pid, cpu_percent, memory_rss_bytes, memory_vsz_bytes, memory_swap_bytes
  - io_read_bytes, io_write_bytes, io_read_rate_bps, io_write_rate_bps
  - threads, minor_faults, major_faults

**Exemplo:**
```bash
./bin/resource-monitor
# Menu > 1
# PID: 1234
# Duração: 60
# Intervalo: 2
# Pressione Ctrl+C para parar
# Gera: monitoring_pid_1234.csv
```

### Componente 2: Namespace Analyzer

Analisa isolamento de namespaces (PID, IPC, MNT, NET, UTS, CGROUP, USER):

```bash
./bin/resource-monitor
# Menu > 2
```

Opções:
1. **Listar namespaces** de um processo específico
2. **Comparar namespaces** entre dois processos
3. **Gerar relatório CSV** do sistema (todos os processos)
4. **Gerar relatório JSON** do sistema

**Exemplo - Listar namespaces:**
```bash
./bin/resource-monitor
# Menu > 2 > 1
# PID: 1
# Mostra: inode de cada namespace para PID 1
```

**Exemplo - Comparar processos:**
```bash
./bin/resource-monitor
# Menu > 2 > 2
# PID 1: 1000
# PID 2: 2000
# Mostra: quais namespaces são compartilhados/isolados
```

**Exemplo - Gerar relatório:**
```bash
./bin/resource-monitor
# Menu > 2 > 3 (CSV) ou 4 (JSON)
# Gera: namespace_report.csv ou namespace_report.json
```

### Componente 3: Control Group Manager

Gerencia limites de recursos via cgroups v2 (requer root):

```bash
sudo ./bin/resource-monitor
# Menu > 3
```

Opções:
1. **Experimento 3** - Throttling de CPU (limites: 0.25, 0.5, 1.0, 2.0 cores)
2. **Experimento 4** - Limitação de Memória (limite: 100MB)
3. **Informações Experimento 5** - Limitação de I/O

**Exemplo - Throttling CPU:**
```bash
sudo ./bin/resource-monitor
# Menu > 3 > 1
# Cria cgroup exp3_test_throttling
# Testa cada limite de CPU
# Exibe métrica: CPU Usage (ns)
# Limpa cgroup ao terminar
```

**Exemplo - Limite de Memória:**
```bash
sudo ./bin/resource-monitor
# Menu > 3 > 2
# Cria cgroup exp4_test_memory
# Aplica limite: 100 MB
# Exibe:
#   - Limite configurado: 100 MB
#   - Uso atual: X MB
#   - Pico: Y MB
#   - Falhas de alocação: N
# Limpa cgroup ao terminar
```

---

## Experimentos Completos (1-5)

### Experimento 1: Overhead de Monitoramento

```bash
sudo ./bin/experimento1_overhead_monitoring
```

Mede o overhead causado pelo Resource Profiler ao monitorar continuamente. Documenta em `docs/experiments/experimento1_overhead.md`.

### Experimento 2: Isolamento de Namespaces

```bash
./bin/experimento2_benchmark_namespaces
```

Realiza benchmark de criação de namespaces em 200 iterações. Gera `namespace_results.csv` com tempos de overhead por tipo (UTS, IPC, MNT, NET, PID, CGROUP, USER).

### Experimento 3: Throttling de CPU

```bash
sudo ./bin/experimento3_throttling_cpu
```

Testa precisão de limites de CPU em cgroups. Documenta em `docs/experiments/experimento3_Throttling de CPU.md`.

### Experimento 4: Limitação de Memória

```bash
sudo ./bin/experimento4_limitacao_memoria
```

Testa limites de memória em cgroups. Documenta em `docs/experiments/experimento4_Limitação de Memória.md`.

### Experimento 5: Limitação de I/O

```bash
sudo ./bin/experimento5_limitacao_io
```

Script externo para testes de limite de I/O em cgroups. Documenta em `docs/experiments/experimento5_limitacao_io.md`.

---

## Testes Unitários

### Teste de CGroup Manager

```bash
# Sem root (alguns testes pulados):
./bin/test_cgroup

# Com root (todos os testes):
sudo ./bin/test_cgroup
```

Executa 6 subtestes:
1. Leitura de métricas do sistema
2. Criar/deletar cgroup
3. Aplicar limite de CPU
4. Aplicar limite de memória
5. Mover processo para cgroup
6. Gerar relatório de utilização

### Teste de CPU Monitor

```bash
./bin/test_cpu
```

Valida cálculos de CPU%.

### Teste de Memory Monitor

```bash
./bin/test_memory
```

Valida leitura de memória.

### Teste de I/O Monitor

```bash
./bin/test_io
```

Valida leitura de I/O.

---

## Formatos de Saída

### CSV (Resource Profiler)

| Coluna | Unidade | Descrição |
|--------|---------|-----------|
| timestamp | YYYY-MM-DD HH:MM:SS | Data/hora |
| pid | - | ID do processo |
| cpu_percent | % | Uso de CPU |
| memory_rss_bytes | bytes | Resident Set Size |
| memory_vsz_bytes | bytes | Virtual Memory |
| memory_swap_bytes | bytes | Swap usado |
| io_read_bytes | bytes | Total leitura |
| io_write_bytes | bytes | Total escrita |
| io_read_rate_bps | B/s | Taxa leitura |
| io_write_rate_bps | B/s | Taxa escrita |
| threads | - | Nº threads |
| minor_faults | - | Page faults menores |
| major_faults | - | Page faults maiores |

### JSON (Namespace Reports)

Estrutura com namespaces de cada processo:
```json
{
  "pid": 1234,
  "command": "process_name",
  "namespaces": {
    "cgroup": "4026531835",
    "ipc": "4026531839",
    "mnt": "4026531840",
    "net": "4026531956",
    "pid": "4026531836",
    "user": "4026531837",
    "uts": "4026531838"
  }
}
```

---

## Exemplos de Uso

### Monitorar Firefox por 60 segundos

```bash
./bin/resource-monitor
# Menu > 1
# PID: $(pgrep firefox)
# Duração: 60
# Intervalo: 1
```

### Comparar isolamento entre dois processos

```bash
./bin/resource-monitor
# Menu > 2 > 2
# PID 1: 1000 (processo A)
# PID 2: 2000 (processo B)
```

### Exportar relatório de namespaces do sistema

```bash
./bin/resource-monitor
# Menu > 2 > 3 (CSV) ou 4 (JSON)
# Arquivos: namespace_report.csv ou namespace_report.json
```

### Aplicar limite de CPU e medir

```bash
sudo ./bin/resource-monitor
# Menu > 3 > 1
# Testa limites: 0.25, 0.5, 1.0, 2.0 cores
```

---

## Tratamento de Privilégios

| Operação | Privilégio |
|----------|-----------|
| Listar/comparar namespaces | Usuário normal |
| Monitorar processo próprio | Usuário normal |
| Monitorar outro processo | root ou mesma permissão |
| Criar/deletar cgroups | root |
| Aplicar limites CPU/Memória/I/O | root |

---

## Troubleshooting

**Erro: "Este experimento precisa de root!"**
```bash
sudo ./bin/resource-monitor
```

**Erro: "Processo com PID X não existe"**
- Listar processos: `ps aux`
- PID pode ter terminado

**Erro: "Permission denied" ao criar cgroup**
- Verificar: `whoami` (deve retornar root)
- Executar: `sudo ./bin/resource-monitor`

**Erro: "cgroup v2 não detectado"**
- Verificar suporte: `mount | grep cgroup2`
- Ativar kernel 6.x+ ou usar distribuição moderna

---

## Autores e Contribuições

**Aluno 2** - Enzo Erkmann - Resource Profiler (I/O), Testes e Experimentos
- Implementação: `src/io_monitor.cpp` (Rede por Processo)
- Benchmark: `tests/experimento5_limitacao_io.cpp`
- Comparação: `scripts/compare_tools.sh`
- Documentação: `docs/experiments/experimento5_limitacao_io.md`

**Aluno 3** - Namespace Analyzer + Experimento 2
- Implementação: `src/namespace_analyzer.cpp` (290 linhas)
- Testes: `tests/experimento2_test_namespaces.cpp`
- Benchmark: `tests/experimento2_benchmark_namespaces.cpp` (300 linhas)
- Experimento 2 com metodologia científica validada em literatura
- Documentação: `docs/experiments/experimento2_namespaces.md`

**Aluno 4** - Integração main.cpp + correções
- Interface principal interativa
- Remoção de emojis
- Timestamps thread-safe
- Tratamento de sinais
- Verificação de retorno de system()

---

**Última atualização**: Novembro 2025

````