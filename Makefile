CC = g++
LD = g++

CPPFLAGS	= -std=gnu++11 -g -Wall -Wconversion -Wextra -Wno-ignored-qualifiers -Wno-write-strings
LIBS		= -libverbs -lpthread


# the rest of the modules should go here.
TPC			:= benchmarks/TPC-C
CLIENT_MODULES	:= $(TPC)/client $(TPC)/client/queries $(TPC)/client/queries/new-order $(TPC)/client/queries/payment $(TPC)/client/queries/order-status $(TPC)/client/queries/stock-level
SERVER_MODULES	:= $(TPC)/server 
TEST_MODULES	:= unit-tests unit-tests/index
INDEX_MODULES	:= index/hash $(TPC)/index-messages
EXPERIMENT_MODULES	:= util executor basic-types rdma-region oracle $(TPC) $(TPC)/random $(TPC)/tables $(SERVER_MODULES) $(CLIENT_MODULES) $(INDEX_MODULES)
UNIT_TEST_MODULES	:= util basic-types rdma-region $(TPC) $(TPC)/random $(TPC)/tables $(INDEX_MODULES) $(TEST_MODULES) 

MODULES		:= $(EXPERIMENT_MODULES)

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
