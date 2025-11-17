#!/usr/bin/env python3
import pandas as pd
import matplotlib.pyplot as plt
import numpy as np
import sys
import os

def main():
    if len(sys.argv) != 2:
        print("Uso: python3 scripts/visualize_namespaces.py <arquivo_csv>")
        print("Exemplo: python3 scripts/visualize_namespaces.py experimento2_benchmark_results.csv")
        sys.exit(1)
    
    csv_file = sys.argv[1]
    
    if not os.path.exists(csv_file):
        print(f"Arquivo não encontrado: {csv_file}")
        print("Execute primeiro: sudo ./bin/experimento2_benchmark_namespaces")
        sys.exit(1)
    
    try:
        df = pd.read_csv(csv_file)
        # Filtrar apenas namespaces suportados
        df = df[df['Supported'] == 'yes']
        print(f"📊 Processando {len(df)} namespaces suportados")
    except Exception as e:
        print(f"Erro ao ler CSV: {e}")
        sys.exit(1)
    
    # Criar gráficos
    fig, (ax1, ax2) = plt.subplots(1, 2, figsize=(15, 6))
    
    # Gráfico 1: Barras com tempo médio
    namespaces = df['Namespace']
    avg_times = df['Avg_us']
    min_times = df['Min_us'] 
    max_times = df['Max_us']
    
    bars = ax1.bar(namespaces, avg_times, color='steelblue', alpha=0.7, edgecolor='black')
    ax1.set_ylabel('Tempo (µs)', fontsize=12)
    ax1.set_xlabel('Namespace', fontsize=12)
    ax1.set_title('Overhead de Criação de Namespaces\n(Tempo Médio)', fontsize=14, fontweight='bold')
    ax1.grid(axis='y', alpha=0.3)
    
    # Adicionar valores nas barras
    for bar, avg, min_val, max_val in zip(bars, avg_times, min_times, max_times):
        height = bar.get_height()
        ax1.text(bar.get_x() + bar.get_width()/2., height + 5,
                f'{avg:.1f}µs\n({min_val:.1f}-{max_val:.1f})',
                ha='center', va='bottom', fontsize=8)
    
    # Gráfico 2: Comparação ranking
    sorted_df = df.sort_values('Avg_us')
    y_pos = np.arange(len(sorted_df))
    
    ax2.barh(y_pos, sorted_df['Avg_us'], color='lightcoral', alpha=0.7, edgecolor='black')
    ax2.set_yticks(y_pos)
    ax2.set_yticklabels(sorted_df['Namespace'])
    ax2.set_xlabel('Tempo Médio (µs)', fontsize=12)
    ax2.set_title('Ranking de Performance\n(do mais rápido ao mais lento)', 
                  fontsize=14, fontweight='bold')
    ax2.grid(axis='x', alpha=0.3)
    
    # Adicionar valores nas barras horizontais
    for i, v in enumerate(sorted_df['Avg_us']):
        ax2.text(v + 5, i, f'{v:.1f}µs', va='center', fontsize=10)
    
    plt.tight_layout()
    plt.savefig('experimento2_overhead.png', dpi=150, bbox_inches='tight')
    print("✅ Gráfico salvo: experimento2_overhead.png")
    
    # Mostrar estatísticas no console
    print(f"\n📈 RESUMO ESTATÍSTICO:")
    print(f"Namespace mais rápido: {sorted_df.iloc[0]['Namespace']} ({sorted_df.iloc[0]['Avg_us']:.1f}µs)")
    print(f"Namespace mais lento: {sorted_df.iloc[-1]['Namespace']} ({sorted_df.iloc[-1]['Avg_us']:.1f}µs)")
    print(f"Razão (lento/rápido): {sorted_df.iloc[-1]['Avg_us']/sorted_df.iloc[0]['Avg_us']:.1f}x")
    
    # Salvar dados para possível uso futuro
    df.to_csv('experimento2_processed.csv', index=False)
    print("📊 Dados processados salvos: experimento2_processed.csv")

if __name__ == "__main__":
    main()
