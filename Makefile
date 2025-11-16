# ============================================================
# MAKEFILE - RESOURCE MONITOR RA3 (CORRIGIDO)
# ============================================================

# Compilador e flags
CXX = g++
CXXFLAGS = -std=c++23 -Wall -Wextra -O2 -I./include
LDFLAGS = -lm

# Diretórios
SRC_DIR = src
INCLUDE_DIR = include
TEST_DIR = tests
BUILD_DIR = build
BIN_DIR = bin

# Arquivos fonte dos componentes EXISTENTES
CPU_MONITOR_SRC = $(SRC_DIR)/cpu_monitor.cpp
MEMORY_MONITOR_SRC = $(SRC_DIR)/memory_monitor.cpp
IO_MONITOR_SRC = $(SRC_DIR)/io_monitor.cpp
NAMESPACE_ANALYZER_SRC = $(SRC_DIR)/namespace_analyzer.cpp
CGROUP_MANAGER_SRC = $(SRC_DIR)/cgroup_manager.cpp
MAIN_SRC = $(SRC_DIR)/main.cpp

# Arquivos objeto
CPU_MONITOR_OBJ = $(BUILD_DIR)/cpu_monitor.o
MEMORY_MONITOR_OBJ = $(BUILD_DIR)/memory_monitor.o
IO_MONITOR_OBJ = $(BUILD_DIR)/io_monitor.o
NAMESPACE_ANALYZER_OBJ = $(BUILD_DIR)/namespace_analyzer.o
CGROUP_MANAGER_OBJ = $(BUILD_DIR)/cgroup_manager.o
MAIN_OBJ = $(BUILD_DIR)/main.o

# Todos os objetos (SEM export_monitor que não existe)
ALL_OBJS = $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ) \
           $(NAMESPACE_ANALYZER_OBJ) $(CGROUP_MANAGER_OBJ)

# Executáveis principais
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
	@echo "==================================="
	@echo "BUILD COMPLETO!"
	@echo "==================================="
	@echo "Binários gerados em $(BIN_DIR)/"
	@echo ""
	@echo "Executáveis principais:"
	@echo "  - $(MAIN_BIN)"
	@echo ""
	@echo "Testes:"
	@echo "  - $(TEST_CPU)"
	@echo "  - $(TEST_MEMORY)"
	@echo "  - $(TEST_IO)"
	@echo "  - $(TEST_CGROUP)"
	@echo ""
	@echo "Experimentos:"
	@echo "  - $(EXP1_BIN)"
	@echo "  - $(EXP2_TEST_BIN)"
	@echo "  - $(EXP2_BENCH_BIN)"
	@echo "  - $(EXP3_BIN)"
	@echo "  - $(EXP4_BIN)"
	@echo "==================================="

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
	@echo "Compilando CPU Monitor..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MEMORY_MONITOR_OBJ): $(MEMORY_MONITOR_SRC)
	@echo "Compilando Memory Monitor..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(IO_MONITOR_OBJ): $(IO_MONITOR_SRC)
	@echo "Compilando I/O Monitor..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(NAMESPACE_ANALYZER_OBJ): $(NAMESPACE_ANALYZER_SRC)
	@echo "Compilando Namespace Analyzer..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(CGROUP_MANAGER_OBJ): $(CGROUP_MANAGER_SRC)
	@echo "Compilando CGroup Manager..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

$(MAIN_OBJ): $(MAIN_SRC)
	@echo "Compilando Main..."
	$(CXX) $(CXXFLAGS) -c $< -o $@

# ============================================================
# EXECUTÁVEL PRINCIPAL
# ============================================================

$(MAIN_BIN): $(ALL_OBJS) $(MAIN_OBJ)
	@echo "Linkando executável principal..."
	$(CXX) $(CXXFLAGS) $(ALL_OBJS) $(MAIN_OBJ) -o $@ $(LDFLAGS)

# ============================================================
# TESTES
# ============================================================

tests: $(TEST_CPU) $(TEST_MEMORY) $(TEST_IO) $(TEST_CGROUP)

$(TEST_CPU): $(TEST_DIR)/test_cpu.cpp
	@echo "Compilando test_cpu..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_MEMORY): $(TEST_DIR)/test_memory.cpp
	@echo "Compilando test_memory..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_IO): $(TEST_DIR)/test_io.cpp
	@echo "Compilando test_io..."
	$(CXX) $(CXXFLAGS) $< -o $@ $(LDFLAGS)

$(TEST_CGROUP): $(TEST_DIR)/test_cgroup.cpp $(CGROUP_MANAGER_OBJ)
	@echo "Compilando test_cgroup..."
	$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

# ============================================================
# EXPERIMENTOS
# ============================================================

experiments: $(EXP1_BIN) $(EXP2_TEST_BIN) $(EXP2_BENCH_BIN) $(EXP3_BIN) $(EXP4_BIN)

$(EXP1_BIN): $(TEST_DIR)/experimento1_overhead_monitoring.cpp $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ)
	@echo "Compilando Experimento 1..."
	$(CXX) $(CXXFLAGS) $< $(CPU_MONITOR_OBJ) $(MEMORY_MONITOR_OBJ) $(IO_MONITOR_OBJ) -o $@ $(LDFLAGS)

$(EXP2_TEST_BIN): $(TEST_DIR)/experimento2_test_namespaces.cpp $(NAMESPACE_ANALYZER_OBJ)
	@echo "Compilando Experimento 2 (testes)..."
	$(CXX) $(CXXFLAGS) $< $(NAMESPACE_ANALYZER_OBJ) -o $@ $(LDFLAGS)

$(EXP2_BENCH_BIN): $(TEST_DIR)/experimento2_benchmark_namespaces.cpp $(NAMESPACE_ANALYZER_OBJ)
	@echo "Compilando Experimento 2 (benchmark)..."
	$(CXX) $(CXXFLAGS) $< $(NAMESPACE_ANALYZER_OBJ) -o $@ $(LDFLAGS)

$(EXP3_BIN): $(TEST_DIR)/experimento3_throttling_cpu.cpp $(CGROUP_MANAGER_OBJ)
	@echo "Compilando Experimento 3 (Throttling CPU)..."
	$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

$(EXP4_BIN): $(TEST_DIR)/experimento4_limitacao_memoria.cpp $(CGROUP_MANAGER_OBJ)
	@echo "Compilando Experimento 4 (Limitação Memória)..."
	$(CXX) $(CXXFLAGS) $< $(CGROUP_MANAGER_OBJ) -o $@ $(LDFLAGS)

# ============================================================
# LIMPEZA
# ============================================================

clean:
	@echo "Limpando arquivos compilados..."
	rm -rf $(BUILD_DIR)
	rm -rf $(BIN_DIR)
	rm -f *.csv *.json *.tmp
	@echo "Limpeza concluída!"

# ============================================================
# TARGETS INDIVIDUAIS (para desenvolvimento)
# ============================================================

cpu_monitor: $(CPU_MONITOR_OBJ)
memory_monitor: $(MEMORY_MONITOR_OBJ)
io_monitor: $(IO_MONITOR_OBJ)
namespace_analyzer: $(NAMESPACE_ANALYZER_OBJ)
cgroup_manager: $(CGROUP_MANAGER_OBJ)

# ============================================================
# HELP
# ============================================================

help:
	@echo "==================================="
	@echo "MAKEFILE - RESOURCE MONITOR RA3"
	@echo "==================================="
	@echo ""
	@echo "Targets disponíveis:"
	@echo "  make all         - Compila tudo (padrão)"
	@echo "  make tests       - Compila apenas os testes"
	@echo "  make experiments - Compila apenas os experimentos"
	@echo "  make clean       - Remove arquivos compilados"
	@echo "  make help        - Mostra esta mensagem"
	@echo ""
	@echo "Targets individuais:"
	@echo "  make cpu_monitor"
	@echo "  make memory_monitor"
	@echo "  make io_monitor"
	@echo "  make namespace_analyzer"
	@echo "  make cgroup_manager"
	@echo "==================================="

.PHONY: all clean tests experiments directories help cpu_monitor memory_monitor io_monitor namespace_analyzer cgroup_manager