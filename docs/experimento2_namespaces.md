# Experimento 2: Namespace Analyzer

**Aluno C**
**Data:** 10/11/2024

---

## 1. Introdução

Este experimento analisa o comportamento dos namespaces do Linux, medindo:
- Efetividade do isolamento entre processos
- Overhead de criação de cada tipo de namespace
- Distribuição de processos por namespace no sistema

## 2. Ambiente de Teste

- **SO:** Ubuntu 24.04 LTS
- **Kernel:** Linux 6.14.0-27-generic
- **CPU:** Intel Core i7-8565U @ 1.80GHz (4 cores, 8 threads)
- **RAM:** 19 GB
- **Compilador:** GCC 13.3.0 com C++23

## 3. Implementação

### Arquivos criados:
- `include/namespace.hpp` - API do analisador
- `src/namespace_analyzer.cpp` - Implementação das 4 funções principais
- `tests/test_namespaces.cpp` - Testes funcionais
- `tests/benchmark_namespaces.cpp` - Medição de overhead

### Funções implementadas:
1. `list_process_namespaces()` - lista namespaces de um processo
2. `find_processes_in_namespace()` - encontra processos em um namespace
3. `compare_namespaces()` - compara namespaces entre processos
4. `generate_namespace_report()` - gera relatório CSV/JSON

### Tipos de namespaces testados:
- CGROUP, IPC, MNT, NET, PID, USER, UTS

## 4. Resultados

### 4.1 Testes Funcionais

**Teste 1 - Listagem:**
```
Listou corretamente os 7 namespaces do processo
Extraiu inodes corretamente
```

**Teste 2 - Busca:**
```
Encontrou 111 processos no mesmo PID namespace
```

**Teste 3 - Comparação:**
```
Comparou namespaces entre processos pai e filho
Identificou corretamente que compartilham todos os namespaces
```

### 4.2 Benchmark de Overhead

**Metodologia:**
- 50 iterações por tipo de namespace
- Medição com `clock_gettime(CLOCK_MONOTONIC)`
- Execução com privilégios root

**Resultados:**

| Namespace | Média (µs) | Mín (µs) | Máx (µs) | Overhead |
|-----------|------------|----------|----------|----------|
| BASELINE  | 735.81     | 532.15   | 1161.06  | -        |
| CGROUP    | 713.33     | 520.93   | 1009.62  | -3.06%   |
| IPC       | 1037.11    | 699.62   | 4565.64  | +40.95%  |
| NET       | 2709.28    | 1776.40  | 4834.84  | +268.20% |
| PID       | 1014.61    | 671.05   | 5659.14  | +37.89%  |
| USER      | 852.06     | 555.80   | 2149.05  | +15.80%  |
| UTS       | 959.07     | 499.38   | 12575.94 | +30.34%  |

### 4.3 Distribuição de Processos

**Processos no namespace default do sistema:**
- CGROUP: 110 processos
- IPC: 102 processos
- NET: 102 processos
- PID: 110 processos

Observação: maioria dos processos compartilha os mesmos namespaces.

## 5. Análise

**Principais observações:**

1. **NET namespace é o mais custoso** (+268% overhead)
   - Precisa criar stack de rede virtual completa
   - Tempo médio: ~2.7ms

2. **USER namespace tem menor overhead** (+15.8%)
   - Não requer root
   - Ideal para containers não-privilegiados
   - Tempo médio: ~850µs

3. **CGROUP teve overhead negativo** (-3.06%)
   - Variação estatística dentro da margem de erro
   - Diferença de apenas 22µs

4. **IPC e PID têm overhead moderado** (~40%)
   - Aceitável para maioria dos casos de uso

5. **Variação alta em alguns casos**
   - UTS teve máximo de 12.5ms (outlier)
   - Provavelmente contenção de recursos

## 6. Conclusões

- Todos os namespaces funcionam corretamente no sistema testado
- Overhead varia de -3% a +268% dependendo do tipo
- NET namespace tem maior custo, USER namespace tem menor
- Para containers, overhead de criação é aceitável (ocorre raramente)
- Isolamento é efetivo - processos não veem recursos de outros namespaces

**Limitações:**
- Apenas overhead de criação foi medido (não runtime)
- MNT namespace não incluído no benchmark
- Resultados podem variar conforme carga do sistema

## 7. Referências

- `man 7 namespaces`
- `man 2 unshare`
- `man 2 clone`
- Kerrisk, M. "Namespaces in operation". LWN.net

---

## Como reproduzir

```bash
# Compilar
g++ -std=c++23 -Wall -Wextra -o test_namespaces \
    tests/test_namespaces.cpp src/namespace_analyzer.cpp

g++ -std=c++23 -Wall -Wextra -o benchmark_namespaces \
    tests/benchmark_namespaces.cpp

# Executar testes
./test_namespaces

# Executar benchmark (precisa root)
sudo ./benchmark_namespaces
```
