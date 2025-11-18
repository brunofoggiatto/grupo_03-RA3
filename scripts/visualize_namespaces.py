#!/usr/bin/env python3
# ============================================================
# ARQUIVO: scripts/visualize_namespaces.py
# DESCRIÇÃO: Script Python para visualizar dados dos experimentos
# Lê arquivo CSV de benchmark de namespaces
# Processa dados e gera gráficos de overhead
# ============================================================

# pandas: biblioteca para manipulação de dados tabulares
# Permite ler CSV, filtrar, ordenar, etc
import pandas as pd

# matplotlib.pyplot: biblioteca para criar gráficos
# Permite plots 2D, barras, linhas, etc
import matplotlib.pyplot as plt

# numpy: biblioteca numérica (operações em arrays)
# Aqui usamos para criar arrays de posições
import numpy as np

# sys: sistema (argumentos de linha de comando, etc)
import sys

# os: sistema operacional (verificar se arquivo existe, etc)
import os

def main():
    # Script espera 1 argumento: nome do arquivo CSV
    if len(sys.argv) != 2:
        # Mostra como usar o script
        print("Uso: python3 scripts/visualize_namespaces.py <arquivo_csv>")
        print("Exemplo: python3 scripts/visualize_namespaces.py experimento2_benchmark_results.csv")
        sys.exit(1)
    
    # Primeiro argumento após o nome do script
    csv_file = sys.argv[1]
    
    if not os.path.exists(csv_file):
        # Arquivo não encontrado
        print(f"Arquivo não encontrado: {csv_file}")
        print("Execute primeiro: sudo ./bin/experimento2_benchmark_namespaces")
        sys.exit(1)
    
    try:
        # Usa pandas para ler CSV em formato tabular
        # pd.read_csv retorna um DataFrame (tabela de dados)
        df = pd.read_csv(csv_file)
        
        # Filtra apenas namespaces suportados 
        # Isto remove namespaces que falharam por permissão ou suporte
        df = df[df['Supported'] == 'yes']
        
        # Mostra quantos namespaces foram processados
        print(f" Processando {len(df)} namespaces suportados")
    except Exception as e:
        # Se houver erro na leitura do CSV
        print(f"Erro ao ler CSV: {e}")
        sys.exit(1)
    
    # Cria uma figura contendo 2 gráficos lado a lado
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
   
    # Obtém colunas específicas do DataFrame
    
    # Nomes dos namespaces 
    namespaces = df['Namespace']
    
    # Tempo médio em microsegundos 
    avg_times = df['Avg_us']
    
    # Tempo mínimo em microsegundos 
    min_times = df['Min_us']
    
    # Tempo máximo em microsegundos 
    max_times = df['Max_us']
    
   
    # Cria gráfico de barras mostrando tempo médio de cada namespace
    
    bars = ax1.bar(namespaces, avg_times, color='steelblue', alpha=0.7, edgecolor='black')
    
    # Rótulos dos eixos
    ax1.set_ylabel('Tempo (µs)', fontsize=12)  # Eixo Y
    ax1.set_xlabel('Namespace', fontsize=12)   # Eixo X
    
    # Título do gráfico
    ax1.set_title('Overhead de Criação de Namespaces\n(Tempo Médio)', fontsize=14, fontweight='bold')
    
    # Grade no eixo Y para facilitar leitura
    ax1.grid(axis='y', alpha=0.3)
    
    # Para cada barra, coloca o valor em cima
   
    for bar, avg, min_val, max_val in zip(bars, avg_times, min_times, max_times):
        # Altura da barra
        height = bar.get_height()
        
        # Coloca texto acima da barra
        ax1.text(
            bar.get_x() + bar.get_width()/2.,  
            height + 5,                        
            f'{avg:.1f}µs\n({min_val:.1f}-{max_val:.1f})',  
            ha='center',  
            va='bottom',  
            fontsize=8    
        )
    
    # Cria gráfico de barras horizontal ordenado do mais rápido ao mais lento
    
    # Ordena dados por tempo médio (do menor para o maior)
    sorted_df = df.sort_values('Avg_us')
    
    # Cria array de posições Y (0, 1, 2, ...)
    # Necessário para barras horizontais
    y_pos = np.arange(len(sorted_df))
    
    # Cria barras horizontais
    ax2.barh(y_pos, sorted_df['Avg_us'], color='lightcoral', alpha=0.7, edgecolor='black')
    
    # Define labels no eixo Y
    # Mostra nomes dos namespaces
    ax2.set_yticks(y_pos)
    ax2.set_yticklabels(sorted_df['Namespace'])
    
    # Rótulos dos eixos
    ax2.set_xlabel('Tempo Médio (µs)', fontsize=12)
    
    # Título
    ax2.set_title('Ranking de Performance\n(do mais rápido ao mais lento)', fontsize=14, fontweight='bold')
    
    # Grade no eixo X
    ax2.grid(axis='x', alpha=0.3)
    
    
    # Para cada barra horizontal, coloca valor no final
    for i, v in enumerate(sorted_df['Avg_us']):
        ax2.text(
            v + 5,   
            i,      
            f'{v:.1f}µs', 
            va='center',  
            fontsize=10
        )
    
    #Ajusta espaçamento entre subplots para evitar sobreposição
    plt.tight_layout()
    plt.savefig('experimento2_overhead.png', dpi=150, bbox_inches='tight')
    print(" Gráfico salvo: experimento2_overhead.png")
    
    # Mostra resumo dos dados (não só gráfico)
    
    print(f"\n RESUMO ESTATÍSTICO:")
    
    # Namespace mais rápido (primeiro em sorted_df)
    print(f"Namespace mais rápido: {sorted_df.iloc[0]['Namespace']} ({sorted_df.iloc[0]['Avg_us']:.1f}µs)")
    
    # Namespace mais lento (último em sorted_df)
    print(f"Namespace mais lento: {sorted_df.iloc[-1]['Namespace']} ({sorted_df.iloc[-1]['Avg_us']:.1f}µs)")
    
    # Razão de performance (quanto mais lento é que mais rápido)
    
    razao = sorted_df.iloc[-1]['Avg_us'] / sorted_df.iloc[0]['Avg_us']
    print(f"Razão (lento/rápido): {razao:.1f}x")
    
   
    # Salva dados em novo CSV para uso futuro
    # Útil se quisermos reanalisar depois
    df.to_csv('experimento2_processed.csv', index=False)
    print(" Dados processados salvos: experimento2_processed.csv")

# __name__ == "__main__" significa "executado como script"
# (não importado como módulo em outro script)
if __name__ == "__main__":
    # Chama função main
    main()
