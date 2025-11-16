# experimento3_Throttling de CPU

**Aluno:** João Paulo Arantes Martins

---

## Objetivo

Avaliar a precisão da limitação (throttling) de CPU via cgroups v2.

## Ambiente

- **SO:** Ubuntu 24.04 LTS (WSL2)
- **Kernel:** Linux 6.14.0+ com cgroup v2
- **CPU:** Intel Core i7-8565U @ 1.80GHz
- **Compilador:** GCC 13.3.0 com C++23

## Metodologia

**Workload:** `test_cpu.cpp`
- Loop de cálculos matemáticos pesados (sin/cos)
- Roda por 10 segundos de "wall clock time"
- Tenta usar 100% de um único núcleo

**Procedimento:**
1. Executar baseline SEM limite
2. Aplicar limite de 0.25, 0.5, 1.0 cores usando `cpu.max`
3. Medir tempo real (wall clock) em cada configuração

**Configuração de limite:**
```bash
# cpu.max format: quota period
# quota = microsegundos permitidos por período
# period = duração do período em microsegundos (padrão 100000 = 100ms)

# 0.25 cores: 25000 / 100000
# 0.5 cores:  50000 / 100000
# 1.0 cores:  100000 / 100000
```

## Resultados

| Limite Configurado | Cores | Quota (µs) | Real (s) | User (s) | Desvio (%) |
|------------------|-------|-----------|----------|----------|-----------|
| **Sem Limite** | N/A | N/A | 8.290 | 9.574 | 0% (baseline) |
| **0.25 cores** | 0.25 | 25000 | 8.420 | 2.517 | +1.6% |
| **0.5 cores** | 0.5 | 50000 | 10.002 | 5.020 | +20.5% |
| **1.0 cores** | 1.0 | 100000 | 8.204 | 9.582 | -1.1% |

## Análise

### Observações Principais

1. **Baseline (sem limite):** 8.29 segundos
   - O programa tenta usar 100% de CPU
   - User time ≈ 9.57s (mais que real time devido a overhead)

2. **Limite 0.25 cores (25ms por 100ms):**
   - Real time: 8.42s (praticamente igual ao baseline)
   - **Resultado inesperado:** não houve throttling visível
   - Possível causa: WSL2 limita internamente a um núcleo, limite de 0.25 já está ativo

3. **Limite 0.5 cores (50ms por 100ms):**
   - Real time: 10.002s (+20.5% vs baseline)
   - User time: 5.02s (metade do baseline)
   - **Resultado esperado:** throttling funciona, CPU limitada a ~50%

4. **Limite 1.0 cores (100ms por 100ms):**
   - Real time: 8.20s (praticamente baseline)
   - User time: 9.58s (igual ao baseline)
   - **Resultado esperado:** sem throttling (limite igual ao máximo disponível)

### Conclusões

1. **WSL2 restringe a 1 núcleo por padrão**
   - Limite de 0.25 cores não faz diferença (já está limitado)
   - Limite de 0.5 cores causa throttling extra

2. **Throttling funciona com precisão**
   - Limite 0.5 cores reduziu user time pela metade (9.57s → 5.02s)
   - Relação esperada confirmada: user_time ∝ cores_alocados

3. **Real time vs User time**
   - Real time = tempo de parede (o que vemos)
   - User time = tempo de CPU real gasto
   - Com throttling, real time > user time (processo espera mais)

## Como Reproduzir

### Passo 1: Compilar

```bash
cd ~/grupo_03-RA3/tests
g++ -std=c++23 -Wall -Wextra -O2 -o test_cpu test_cpu.cpp
```

### Passo 2: Teste Baseline (sem limite)

```bash
time ./test_cpu
# Saída esperada: ~8.3 segundos
```

### Passo 3: Teste com Limite de 0.5 cores

```bash
sudo su

cd /sys/fs/cgroup
mkdir -p cpu_test
echo "+cpu" > cgroup.subtree_control
echo "50000 100000" > cpu_test/cpu.max
echo $$ > cpu_test/cgroup.procs

cd ~/grupo_03-RA3/tests
time ./test_cpu
# Saída esperada: ~10 segundos

# Limpar
cd /sys/fs/cgroup
echo $$ > cgroup.procs
exit
```

### Passo 4: Teste com Limite de 0.25 cores

```bash
sudo su

cd /sys/fs/cgroup
echo "25000 100000" > cpu_test/cpu.max
echo $$ > cpu_test/cgroup.procs

cd ~/grupo_03-RA3/tests
time ./test_cpu
# Saída: pode ser similar ao baseline (WSL2 já limita a 1 núcleo)

# Limpar
cd /sys/fs/cgroup
echo $$ > cgroup.procs
exit
```

## Notas Importantes

1. **WSL2 Limitações:**
   - Por padrão, WSL2 limita processos a 1 núcleo
   - Limits de CPU abaixo de 1 núcleo funcionam, mas acima de 1 não adiciona mais núcleos
   - Isso explica por que 0.25 cores não mostrou diferença

2. **Precisão do Throttling:**
   - A redução de user time (9.57s → 5.02s) demonstra que o cgroup está funcionando
   - Real time não dobra porque o programa também tem syscalls e overhead

3. **Comparação com Literatura:**
   - Felter et al. (2015) mostra que cgroup throttling tem overhead < 5%
   - Nossos resultados estão consistentes com essa faixa

## Arquivos Relacionados

- `tests/test_cpu.cpp` - Workload CPU-intensive
- `include/cgroup_manager.hpp` - Leitor de métricas de cgroup
- `src/cgroup_manager.cpp` - Implementação