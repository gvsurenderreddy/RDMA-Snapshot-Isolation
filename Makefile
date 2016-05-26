CC = g++
LD = g++

CPPFLAGS	= -std=gnu++11 -g -Wall -Wconversion -Wextra -Wno-ignored-qualifiers -Wno-write-strings
LIBS		= -libverbs -lpthread


# the rest of the modules should go here.
TPC			:= benchmarks/TPC-C
RDMA_TPC	:= $(TPC)/one-sided-architecture
QUERIES		:= $(RDMA_TPC)/client/queries
CLIENT_MODULES	:= $(RDMA_TPC)/client
QUERIES_MODILES	:= $(QUERIES) $(QUERIES)/new-order $(QUERIES)/payment $(QUERIES)/order-status $(QUERIES)/delivery $(QUERIES)/stock-level
SERVER_MODULES	:= $(RDMA_TPC)/server 
ORACLE_MODULES	:= oracle oracle/vector-snapshot-oracle
INDEX_MODULES	:= index/hash $(RDMA_TPC)/index-messages

#TwoPC_TPC	:= $(TPC)/2PC-architecture
#ORACLE_MODULES	:= oracle oracle/single-rts-oracle oracle/vector-snapshot-oracle
#CLIENT_MODULES	:= $(TwoPC_TPC)/client
#SERVER_MODULES	:= $(TwoPC_TPC)/server 
#QUERIES_MODULES	:= $(TwoPC_TPC)/queries
#INDEX_MODULES	:= index/queue

EXPERIMENT_MODULES	:= util executor basic-types rdma-region recovery $(ORACLE_MODULES) $(TPC) $(TPC)/random $(TPC)/tables $(SERVER_MODULES) $(CLIENT_MODULES) $(QUERIES_MODILES) $(QUERIES_MODULES) $(INDEX_MODULES)

TEST_MODULES	:= unit-tests unit-tests/index unit-tests/TPCC unit-tests/basic-types
UNIT_TEST_MODULES	:= util basic-types rdma-region $(TPC) $(TPC)/random $(TPC)/tables $(INDEX_MODULES) $(TEST_MODULES) 

MODULES		:= $(EXPERIMENT_MODULES)		# set to either EXPERIMENT_MODULES or UNIT_TEST_MODULES

SRC_DIR		:= $(addprefix src/,$(MODULES))
BUILD_DIR	:= $(addprefix build/,$(MODULES))
EXE_DIR		:= exe
LOG_DIR		:= logs

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))
OBJ			:= $(patsubst src/%.cpp,build/%.o,$(SRC))
INCLUDES	:= $(addprefix -I,$(SRC_DIR))


# -------------------------------------------------
# telling the make to seach for cpp files in these folders
vpath %.cpp $(SRC_DIR)

# -------------------------------------------------
# universal compilie commands
define make-goal
$1/%.o: %.cpp
	$(CC) $(CPPFLAGS) $(INCLUDES) -c $$< -o $$@
endef

.PHONY: all checkdirs clean

# -------------------------------------------------
all: checkdirs $(EXE_DIR)/executor.exe

# -------------------------------------------------
# linking all object files to create the executable
$(EXE_DIR)/executor.exe: $(OBJ)
	$(LD) $^ $(LIBS) -o $@

# -------------------------------------------------
# Creating needed folders if don't exist
checkdirs: $(BUILD_DIR) $(EXE_DIR) $(LOG_DIR)

$(BUILD_DIR):
	@mkdir -p $@

$(EXE_DIR):
	@mkdir -p $@

$(LOG_DIR):
	@mkdir -p $@

# -------------------------------------------------
# Cleaning the object files
clean:
	rm -rf $(BUILD_DIR)
	

# this line creates the implicit rules for each build using the function make-goal,
# and it is not necessary write them one by one
$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))
