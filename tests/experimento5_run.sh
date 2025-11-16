#!/bin/bash
# -----------------------------------------------------------------------------
# ARQUIVO: experimento5_run.sh
#
# PROPÓSITO:
# Automatiza o Experimento 5. Este script é inteligente e detecta
# automaticamente o disco (MAJ:MIN) onde ele está sendo executado.
#
# RESPONSABILIDADE: Aluno 2
#
# LÓGICA DE FUNCIONAMENTO:
# 1. Acha o disco (MAJ:MIN) do diretório atual (.).
# 2. Configura o cgroup v2.
# 3. Roda o workload 'test_io' (que deve estar nesta mesma pasta).
# 4. Aplica os limites de 1MB/s e 5MB/s e roda o workload de novo.
#
# COMO EXECUTAR (Exemplo):
# 1. Compile: g++ -std=c++23 -o test_io test_io.cpp
# 2. Dê permissão: chmod +x experimento5_run.sh
# 3. Rode com sudo: sudo ./experimento5_run.sh
# -----------------------------------------------------------------------------

# --- VARIÁVEIS DE CONFIGURAÇÃO ---
CGROUP_PATH="/sys/fs/cgroup"
MY_GROUP="io_limit"
MY_GROUP_PATH="$CGROUP_PATH/$MY_GROUP"

# O workload DEVE estar na mesma pasta deste script
WORKLOAD_EXEC="./test_io"

# Limites em bytes
LIMIT_1MB=1048576
LIMIT_5MB=5242880

# --- FUNÇÃO DE LOG ---
log() {
    echo "--- $1 ---"
}

# --- FUNÇÃO PRINCIPAL ---
main() {
    log "INICIANDO EXPERIMENTO 5: LIMITAÇÃO DE I/O"
    
    # 1. VERIFICAR SUDO
    if [ "$EUID" -ne 0 ]; then
        echo "ERRO: Este script precisa ser executado com sudo."
        exit 1
    fi

    # 2. VERIFICAR WORKLOAD
    if [ ! -f "$WORKLOAD_EXEC" ]; then
        echo "ERRO: Workload '$WORKLOAD_EXEC' não encontrado."
        echo "Execute 'g++ -std=c++23 -o test_io test_io.cpp' nesta pasta primeiro."
        exit 1
    fi

    # 3. ACHAR O DISCO AUTOMATICAMENTE
    log "Identificando o disco principal..."
    # Acha a "fonte" (partição) onde o diretório atual (.) está
    DEVICE_PATH=$(findmnt -n -o SOURCE --target .)
    # Usa o lsblk para pegar os números MAJ:MIN daquele device
    TARGET_DISK=$(lsblk -dno MAJ:MIN "$DEVICE_PATH")

    if [ -z "$TARGET_DISK" ]; then
        echo "ERRO: Não consegui identificar o disco (MAJ:MIN) automaticamente."
        exit 1
    fi
    log "Disco alvo identificado: $TARGET_DISK (Device: $DEVICE_PATH)"


    # --- 4. PREPARAÇÃO DO CGROUP V2 ---
    log "Configurando ambiente cgroup v2..."
    mkdir -p "$MY_GROUP_PATH"
    echo "+io" > "$CGROUP_PATH/cgroup.subtree_control"
    echo ""


    # --- 5. TESTE BASELINE (SEM LIMITE) ---
    log "Executando Baseline (Sem Limite)..."
    echo $$ > "$CGROUP_PATH/cgroup.procs"
    $WORKLOAD_EXEC
    echo ""


    # --- 6. TESTE COM LIMITE (1 MB/s) ---
    log "Executando Teste com Limite de 1 MB/s..."
    echo "$TARGET_DISK wbps=$LIMIT_1MB" > "$MY_GROUP_PATH/io.max"
    echo $$ > "$MY_GROUP_PATH/cgroup.procs"
    $WORKLOAD_EXEC
    echo ""


    # --- 7. TESTE COM LIMITE (5 MB/s) ---
    log "Executando Teste com Limite de 5 MB/s..."
    echo "$TARGET_DISK wbps=$LIMIT_5MB" > "$MY_GROUP_PATH/io.max"
    echo $$ > "$MY_GROUP_PATH/cgroup.procs"
    $WORKLOAD_EXEC
    echo ""


    # --- LIMPEZA ---
    log "Limpando... (Movendo de volta ao cgroup raiz)"
    echo $$ > "$CGROUP_PATH/cgroup.procs"
    
    log "EXPERIMENTO 5 CONCLUÍDO."
}

# --- EXECUTA O SCRIPT ---
main