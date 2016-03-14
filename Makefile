CC = g++
LD = g++

CPPFLAGS	= -std=gnu++11 -g -Wall -Wconversion -Wextra -Wno-ignored-qualifiers -Wno-write-strings
LIBS		= -libverbs -lpthread


# all the agents (including subfolders) should go here
#AGENTS_MODULES		:= rdma-SI rdma-SI/client rdma-SI/server rdma-SI/timestamp-oracle
AGENTS_MODULES		:= TSM-SI TSM-SI/client TSM-SI/server TSM-SI/timestamp-oracle


# agents for benchmarking
BENCHMARK_MODULES	:= micro-benchmarks micro-benchmarks/simple-verbs

TPC			:= benchmarks/TPC-C

# the rest of the modules should go here.
#MODULES		:= util basic-types executor $(AGENTS_MODULES)
#MODULES		:= util auxilary $(BENCHMARK_MODULES)
UNIT_TESTS	:= unit-tests unit-tests/tpcw-tests 
MODULES		:= util executor basic-types rdma-region oracle $(TPC) $(TPC)/random $(TPC)/tables $(TPC)/server $(TPC)/client #$(UNIT_TESTS) 

SRC_DIR		:= $(addprefix src/,$(MODULES))
BUILD_DIR	:= $(addprefix build/,$(MODULES))
EXE_DIR		:= exe

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
all: checkdirs $(EXE_DIR)/test.exe

# -------------------------------------------------
# linking all object files to create the executable
$(EXE_DIR)/test.exe: $(OBJ)
	$(LD) $^ $(LIBS) -o $@

# -------------------------------------------------
# Creating needed folders if don't exist
checkdirs: $(BUILD_DIR) $(EXE_DIR)

$(BUILD_DIR):
	@mkdir -p $@

$(EXE_DIR):
	@mkdir -p $@
# -------------------------------------------------
# Cleaning the object files
clean:
	#rm -f $(OBJ)
	rm -rf $(BUILD_DIR)
	

# this line creates the implicit rules for each build using the function make-goal,
# and it is not necessary write them one by one
$(foreach bdir,$(BUILD_DIR),$(eval $(call make-goal,$(bdir))))
