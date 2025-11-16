# ============================================================
# MAKEFILE - RESOURCE MONITOR RA3 (VERSÃO CORRIGIDA)
# ============================================================

CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2 -I./include
LDFLAGS = -lm

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# ============================================================
# ARQUIVOS FONTE DOS COMPONENTES
# ============================================================

# COMPONENTE 1: Resource Profiler
CPU_MONITOR_SRC = $(SRC_DIR)/cpu_monitor.cpp
MEMORY_MONITOR_SRC = $(SRC_DIR)/memory_monitor.cpp
IO_MONITOR_SRC = $(SRC_DIR)/io_monitor.cpp

# COMPONENTE 2: Namespace Analyzer
NAMESPACE_ANALYZER_SRC = $(SRC_DIR)/namespace_analyzer.cpp

# COMPONENTE 3: Control Group Manager
CGROUP_MANAGER_SRC = $(SRC_DIR)/cgroup_manager.cpp

# Main (Integra todos os componentes)
MAIN_SRC = $(SRC_DIR)/main.cpp

# ============================================================
# ARQUIVOS OBJETO
# ============================================================

CPU_MONITOR_OBJ = $(BUILD_DIR)/cpu_monitor.o
MEMORY_MONITOR_OBJ = $(BUILD_DIR)/memory_monitor.o
IO_MONITOR_OBJ = $(BUILD_DIR)/io_monitor.o
NAMESPACE_ANALYZER_OBJ = $(BUILD_DIR)/namespace_analyzer.o
CGROUP_MANAGER_OBJ = $(BUILD_DIR)/cgroup_manager.o
MAIN_OBJ = $(BUILD_DIR)/main.o

# Todos os objetos
ALL_OBJS = $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ) \
           $(NAMESPACE_ANALYZER_OBJ) $(CGROUP_MANAGER_OBJ)

# ============================================================
# EXECUTÁVEIS
# ============================================================

MAIN_BIN = $(BIN_DIR)/resource-monitor

# Testes
TEST_CPU = $(BIN_DIR)/test_cpu
TEST_MEMORY = $(BIN_DIR)/test_memory
TEST_IO = $(BIN_DIR)/test_io
TEST_CGROUP = $(BIN_DIR)/test_cgroup

# Experimentos
EXP1_BIN = $(BIN_DIR)/experimento1_overhead_monitoring
EXP2_TEST_BIN = $(BIN_DIR)/experimento2_test_namespaces
EXP2_BENCH_BIN = $(BIN_DIR)/experimento2_benchmark_namespaces
EXP3_BIN = $(BIN_DIR)/experimento3_throttling_cpu
EXP4_BIN = $(BIN_DIR)/experimento4_limitacao_memoria

# ============================================================
# TARGET PRINCIPAL
# ============================================================

all: directories $(MAIN_BIN) tests experiments
	@echo ""
	@echo "╔═══════════════════════════════════════════╗"
	@echo "║   BUILD COMPLETO - RA3 RESOURCE MONITOR  ║"
	@echo "╚═══════════════════════════════════════════╝"
	@echo ""
	@echo "COMPONENTES INTEGRADOS:"
	@echo "   • Componente 1: Resource Profiler"
	@echo "   • Componente 2: Namespace Analyzer"
	@echo "   • Componente 3: Control Group Manager"
	@echo ""
	@echo "Executáveis principais:"
	@echo "   $(MAIN_BIN)"
	@echo ""
	@echo "Testes:"
	@echo "   $(TEST_CPU)"
	@echo "   $(TEST_MEMORY)"
	@echo "   $(TEST_IO)"
	@echo "   $(TEST_CGROUP)"
	@echo ""
	@echo "Experimentos:"
	@echo "   $(EXP1_BIN)"
	@echo "   $(EXP2_TEST_BIN)"
	@echo "   $(EXP2_BENCH_BIN)"
	@echo "   $(EXP3_BIN)"
	@echo "   $(EXP4_BIN)"
	@echo ""

# ============================================================
# CRIAR DIRETÓRIOS
# ============================================================

directories:
	@mkdir -p $(BUILD_DIR)
	@mkdir -p $(BIN_DIR)

# ============================================================
# COMPILAR COMPONENTES (OBJETOS)
# ============================================================

$(CPU_MONITOR_OBJ): $(CPU_MONITOR_SRC)
	@echo "📝 Compilando CPU Monitor..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(MEMORY_MONITOR_OBJ): $(MEMORY_MONITOR_SRC)
	@echo "📝 Compilando Memory Monitor..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(IO_MONITOR_OBJ): $(IO_MONITOR_SRC)
	@echo "📝 Compilando I/O Monitor..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAMESPACE_ANALYZER_OBJ): $(NAMESPACE_ANALYZER_SRC)
	@echo "📝 Compilando Namespace Analyzer..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(CGROUP_MANAGER_OBJ): $(CGROUP_MANAGER_SRC)
	@echo "📝 Compilando CGroup Manager..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_SRC)
	@echo "📝 Compilando Main (Integração)..."
	@$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================
# EXECUTÁVEL PRINCIPAL
# ============================================================

$(MAIN_BIN): $(ALL_OBJS) $(MAIN_OBJ)
	@echo ""
	@echo "🔗 Linkando executável principal..."
	@$(CXX) $(CXXFLAGS) $(ALL_OBJS) $(MAIN_OBJ) -o $@ $(LDFLAGS)
	@echo "Executável principal criado: $@"

# ============================================================
# TESTES
# ============================================================

tests: $(TEST_CPU) $(TEST_MEMORY) $(TEST_IO) $(TEST_CGROUP)
	@echo "Testes compilados com sucesso"

$(TEST_CPU): $(TEST_DIR)/test_cpu.cpp
	@echo "📝 Compilando test_cpu..."
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_MEMORY): $(TEST_DIR)/test_memory.cpp
	@echo "📝 Compilando test_memory..."
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_IO): $(TEST_DIR)/test_io.cpp
	@echo "📝 Compilando test_io..."
	@$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_CGROUP): $(TEST_DIR)/test_cgroup.cpp $(CGROUP_MANAGER_OBJ)
	@echo "📝 Compilando test_cgroup..."
	@$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

# ============================================================
# EXPERIMENTOS
# ============================================================

experiments: $(EXP1_BIN) $(EXP2_TEST_BIN) $(EXP2_BENCH_BIN) $(EXP3_BIN) $(EXP4_BIN)
	@echo "Experimentos compilados com sucesso"

$(EXP1_BIN): $(TEST_DIR)/experimento1_overhead_monitoring.cpp $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ)
	@echo "📝 Compilando Experimento 1..."
	@$(CXX) $(CXXFLAGS) $< $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ) -o $@ $(LDFLAGS)

$(EXP2_TEST_BIN): $(TEST_DIR)/experimento2_test_namespaces.cpp $(NAMESPACE_ANALYZER_OBJ)
	@echo "📝 Compilando Experimento 2 (testes)..."
	@$(CXX) $(CXXFLAGS) $< $(NAMESPACE_ANALYZER_OBJ) -o $@ $(LDFLAGS)

$(EXP2_BENCH_BIN): $(TEST_DIR)/experimento2_benchmark_namespaces.cpp $(NAMESPACE_ANALYZER_OBJ)
	@echo "📝 Compilando Experimento 2 (benchmark)..."
	@$(CXX) $(CXXFLAGS) $< $(NAMESPACE_ANALYZER_OBJ) -o $@ $(LDFLAGS)

$(EXP3_BIN): $(TEST_DIR)/experimento3_throttling_cpu.cpp $(CGROUP_MANAGER_OBJ)
	@echo "📝 Compilando Experimento 3..."
	@$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

$(EXP4_BIN): $(TEST_DIR)/experimento4_limitacao_memoria.cpp $(CGROUP_MANAGER_OBJ)
	@echo "📝 Compilando Experimento 4..."
	@$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

# ============================================================
# LIMPEZA
# ============================================================

clean:
	@echo "🗑️  Limpando arquivos compilados..."
	@rm -rf $(BUILD_DIR)
	@rm -rf $(BIN_DIR)
	@rm -f *.csv *.json *.tmp
	@echo "Limpeza concluída!"

# ============================================================
# EXECUTAR
# ============================================================

run: all
	@echo ""
	@echo "🚀 Executando Resource Monitor..."
	@echo ""
	@./$(MAIN_BIN)

run-root: all
	@echo ""
	@echo "🚀 Executando Resource Monitor com sudo..."
	@echo ""
	@sudo ./$(MAIN_BIN)

# ============================================================
# AJUDA
# ============================================================

help:
	@echo "╔════════════════════════════════════════════╗"
	@echo "║  MAKEFILE - RESOURCE MONITOR RA3          ║"
	@echo "║  Componentes 1, 2, 3 Integrados           ║"
	@echo "╚════════════════════════════════════════════╝"
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  make              - Compila tudo"
	@echo "  make clean        - Remove compilação"
	@echo "  make run          - Compila e executa"
	@echo "  make run-root     - Compila e executa com sudo"
	@echo "  make help         - Mostra esta mensagem"
	@echo ""

.PHONY: all clean run run-root help tests experiments directories