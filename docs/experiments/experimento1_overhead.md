# Experimento 1: Overhead de Monitoramento

**Aluno:** Aluno C

---

## Objetivo

Medir o impacto do próprio profiler (Resource Profiler - Componente 1) no sistema.

## Ambiente

- **SO:** Ubuntu 24.04 LTS
- **Kernel:** Linux 6.14.0-27-generic
- **CPU:** Intel Core i7-8565U @ 1.80GHz
- **Compilador:** GCC 13.3.0 com C++23

## Procedimento

1. Executar workload de referência **sem monitoramento**
2. Executar mesmo workload **com monitoramento** em diferentes intervalos (10ms, 50ms, 100ms, 500ms)
3. Medir diferenças em CPU usage e execution time

**Workload:** Cálculo matemático intensivo (5 milhões de iterações)

**Métricas coletadas pelo profiler:**
- CPU (usando `get_cpu_usage()`)
- Memória (usando `get_memory_usage()`)
- I/O (usando `get_io_usage()`)

## Resultados

| Configuração | Tempo (ms) | Overhead (%) | Latência (µs) |
|--------------|------------|--------------|---------------|
| Sem monitoramento | 537.07 | 0.00% | - |
| Intervalo 10ms | 705.74 | +31.41% | 559 |
| Intervalo 50ms | 608.10 | +13.23% | 479 |
| Intervalo 100ms | 604.10 | +12.48% | 424 |
| Intervalo 500ms | 1002.14 | +86.59% | 558 |

## Análise

- Overhead máximo: **+86.59%** (intervalo 500ms)
- Overhead mínimo: **+12.48%** (intervalo 100ms)
- Overhead ótimo: **+13.23%** (intervalo 50ms)
- Latência de coleta: **~505µs** (média consistente)

**Conclusões:**

1. **Intervalo 100ms é o mais eficiente** (~12.5% overhead)
2. **Intervalo 10ms tem overhead alto** (+31%) devido a chamadas frequentes ao `/proc`
3. **Intervalo 500ms tem overhead muito alto** (+86%) porque o pai "dorme" demais, atrasando a detecção do término do filho
4. **Sweet spot: 50-100ms** - balanceia frequência de amostragem e overhead

## Como reproduzir

```bash
# Compilar
g++ -std=c++23 -Wall -Wextra -o experimento1_overhead_monitoring \
    tests/experimento1_overhead_monitoring.cpp \
    src/cpu_monitor.cpp \
    src/memory_monitor.cpp \
    src/io_monitor.cpp

# Executar
./experimento1_overhead_monitoring
```
