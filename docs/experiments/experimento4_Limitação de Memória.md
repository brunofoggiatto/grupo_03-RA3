# Experimento 4: Limitação de Memória

**Aluno:** João Paulo Arantes Martins

---

## Objetivo

Testar o comportamento do sistema ao atingir um limite de memória imposto por cgroup, medindo precisão e falhas de alocação.

## Ambiente

- **SO:** Ubuntu 24.04 LTS (WSL2)
- **Kernel:** Linux 6.14.0+ com cgroup v2
- **RAM Disponível:** ~8GB
- **Compilador:** GCC 13.3.0 com C++23

## Metodologia

**Workload:** `test_memory.cpp`
- Aloca memória em blocos de 10MB
- Total: 50 blocos × 10MB = 500MB pretendidos
- Pausa 200ms entre blocos para permitir monitoramento
- Mantém alocado por 10 segundos

**Procedimento:**
1. Criar cgroup com limite de 100MB
2. Executar `test_memory.cpp` dentro do cgroup
3. Monitorar:
   - `memory.current` - memória atual usada
   - `memory.peak` - pico máximo atingido
   - `memory.events[max]` - número de falhas de alocação
4. Observar comportamento do kernel (sem OOM killer neste caso)

**Configuração:**
```bash
# Limite de 100MB
echo "104857600" > /sys/fs/cgroup/memory_test/memory.max
# 104857600 bytes = 100 * 1024 * 1024
```

## Resultados

### Métricas Coletadas

| Métrica | Valor | Interpretação |
|---------|-------|----------------|
| **Limite configurado** | 100 MB | Teto máximo permitido |
| **Memory Peak** | 100 MB | Pico atingido = limite exato |
| **Memory Current (final)** | 676 KB | Memória ainda alocada ao fim |
| **Alocação Total Tentada** | 500 MB | Programa tentou alocar 500MB |
| **Alocação Bem-sucedida** | ~100 MB | Apenas o limite foi alocado |
| **Falhas de Alocação** | 2733 | Número de `malloc()` falhados |

### Análise Detalhada

#### 1. Memory Peak = Limite Exato

```
memory.peak: 104857600 bytes = 100 MB
memory.max:  104857600 bytes = 100 MB
```

**Significado:** O kernel permitiu que o programa alocasse exatamente até o limite e depois bloqueou novas alocações. Não houve OOM kill porque o programa tratou as falhas de alocação graciosamente (com `try-catch`).

#### 2. Falhas de Alocação: 2733

O `test_memory.cpp` tenta alocar 50 blocos de 10MB cada. Após ~10 blocos alocados com sucesso (100MB), o kernel nega as próximas alocações.

**Cálculo esperado:**
- Blocos bem-sucedidos: ~10 (100MB ÷ 10MB/bloco)
- Blocos falhados: ~40 (500MB - 100MB)
- Cada falha de alocação incrementa `memory.events[max]`
- Valor 2733 = múltiplas tentativas dentro de cada `new` (alocações pequenas internas)

#### 3. Memory Current ≠ Memory Peak

```
memory.peak:   104857600 bytes (100 MB)
memory.current: 692224 bytes (~676 KB)
```

**Por quê?** O programa libera a memória ao final (no destrutor do `std::vector`). O que permanece é overhead do kernel.

### Comportamento Observado

**Sem OOM killer:**
- O programa NÃO foi morto
- As alocações simplesmente falharam silenciosamente
- O kernel retornou `nullptr` para `new` (em C++)

**Com OOM killer ativo (se fosse usado `malloc` com menos tratamento):**
- O processo teria sido terminado assim que atingisse 100MB
- Veríamos um crash ou `signal 9`

## Como Reproduzir

### Passo 1: Compilar

```bash
cd ~/grupo_03-RA3/tests
g++ -std=c++23 -Wall -Wextra -O2 -o test_memory test_memory.cpp
```

### Passo 2: Criar cgroup com limite

```bash
sudo su

cd /sys/fs/cgroup
mkdir -p memory_test
echo "104857600" > memory_test/memory.max  # 100MB

echo $$ > memory_test/cgroup.procs  # Move shell para o cgroup
```

### Passo 3: Executar teste

```bash
cd ~/grupo_03-RA3/tests
timeout 15 ./test_memory
```

Saída esperada:
```
Iniciando teste de memória (alocando 500 MB)...
Mantendo memória alocada por 10 segundos...
Memória liberada, teste concluído.
```

### Passo 4: Coletar métricas

```bash
cat /sys/fs/cgroup/memory_test/memory.current
cat /sys/fs/cgroup/memory_test/memory.peak
cat /sys/fs/cgroup/memory_test/memory.events
```

Esperado:
```
memory.current: ~692224 (pode variar)
memory.peak:    104857600 (sempre 100MB)
memory.max:     2733 (falhas de alocação)
```

### Passo 5: Limpar

```bash
cd /sys/fs/cgroup
echo $$ > cgroup.procs  # Sair do cgroup
exit
```

## Notas Importantes

### 1. Diferença entre memory.failcnt e memory.events[max]

- **cgroup v1:** usa `memory.failcnt`
- **cgroup v2 (nosso caso):** usa `memory.events[max]`

Ambos contabilizam falhas, mas com nomes diferentes.

### 2. Não houve OOM Killer

O programa **não foi terminado** porque:
1. Tratou exceções de alocação graciosamente
2. O `std::vector::allocator` retornou `nullptr`
3. O programa saiu naturalmente

Se usássemos `malloc` com menos tratamento de erro, o OOM killer teria agido.

### 3. Memory Peak vs Memory Current

- **Peak:** maior quantidade já usada (atingiu o limite)
- **Current:** quantidade em uso agora (menor porque liberou)

Isso mostra que o limite foi **efetivamente imposto e respeitado**.

### 4. Precisão do Limite

O limite foi **100% preciso:**
- Configurado: 100 MB (104857600 bytes)
- Pico atingido: 100 MB (104857600 bytes)
- Diferença: 0 bytes

## Comparação com Literatura

Segundo Felter et al. (2015) e Red Hat Documentation:
- Cgroups v2 memory limiting é **altamente preciso**
- Overhead < 2%
- Nossos resultados confirmam essa precisão

## Conclusões

1. ✅ **Limite funciona com precisão:** programa alocou exatamente até o limite
2. ✅ **Falhas contabilizadas:** 2733 tentativas falhadas registradas
3. ✅ **Sem OOM killer:** programa não foi morto (tratamento gracioso)
4. ✅ **Comportamento previsível:** pico = limite, falhas = alocações bloqueadas

## Arquivos Relacionados

- `tests/test_memory.cpp` - Workload memory-intensive
- `include/cgroup_manager.hpp` - Leitor de métricas (suporta memory.peak, memory.failcnt)
- `src/cgroup_manager.cpp` - Implementação
- `experimento5_limitacao_io.md` - Experimento similar para I/O