# resource-monitor

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


