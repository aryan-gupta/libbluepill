#===============================================================================
#    @author          Aryan Gupta <me@theguptaempire.net>
#    @title           Makefile
#    @brief           This is a Makefile I created for my STM32 Blue Pill projects
#===============================================================================
.DEFAULT_GOAL := all
#==========================  CONFIG MACROS  ====================================
PREFIX  = arm-none-eabi-
CC     := $(PREFIX)gcc
CXX    := $(PREFIX)g++
LD     := $(PREFIX)gcc
AR     := $(PREFIX)ar
AS     := $(PREFIX)as
OBJCOPY := $(PREFIX)objcopy
SIZE    := $(PREFIX)size
OBJDUMP := $(PREFIX)objdump
GDB     := $(PREFIX)gdb
STFLASH	= $(shell which st-flash)
PRGM = main

# Directories
BIN = bin
SRC = src
DEP = dep

# Final executable
ELF = $(BIN)/$(PRGM).elf
BINARY = $(BIN)/$(PRGM).bin
MAP = $(BIN)/$(PRGM).map

WARNINGS = -Wshadow -Wextra -Wredundant-decls -Weffc++ -Wall -Wundef
ARCH_FLAGS = -mthumb -mcpu=cortex-m3 -msoft-float -mfix-cortex-m3-ldrd
FOPT = -fno-common -ffunction-sections -fdata-sections
LDSCRIPT = stm32f103c8t6.ld

# libopencm3
OPENCM3_DIR = ../libraries/libopencm3
OPENCM3_INC = -I$(OPENCM3_DIR)/include
OPENCM3_LNK = -L$(OPENCM3_DIR)/lib -lopencm3_stm32f1
DEFS        = -DSTM32F1

# FreeRTOS
FREERTOS_DIR = ../libraries/FreeRTOSv10.2.1/FreeRTOS/Source
FREERTOS_PORT_DIR = $(FREERTOS_DIR)/portable/GCC/ARM_CM3
FREERTOS_MEM_DIR =  $(FREERTOS_DIR)/portable/MemMang
FREERTOS_INC = -isystem $(FREERTOS_DIR)/include -Isrc -isystem $(FREERTOS_PORT_DIR) -isystem $(FREERTOS_MEM_DIR)

LDLIBS = -Wl,--start-group -lsupc++ -lc -lgcc -lnosys -Wl,--end-group -specs=nosys.specs
LDFLAGS	= -Wall --static -nostartfiles -T$(LDSCRIPT) $(ARCH_FLAGS) -Wl,-Map=$(MAP) -Wl,--cref -Wl,--gc-sections

# Boost libs
# BOOST_INC = -I/usr/local/boost/include
# BOOST_LNK = -L/usr/local/boost/lib -lboost_atomic -lboost_graph -lboost_math_tr1l -lboost_stacktrace_noop \
  -lboost_chrono -lboost_iostreams -lboost_prg_exec_monitor -lboost_system -lboost_container -lboost_locale \
  -lboost_program_options -lboost_test_exec_monitor -lboost_context -lboost_log -lboost_random -lboost_thread \
  -lboost_contract -lboost_log_setup -lboost_regex -lboost_timer -lboost_coroutine -lboost_math_c99 \
  -lboost_serialization -lboost_type_erasure -lboost_date_time -lboost_math_c99f -lboost_signals \
  -lboost_unit_test_framework -lboost_exception -lboost_math_c99l -lboost_stacktrace_addr2line -lboost_wave \
  -lboost_fiber -lboost_math_tr1 -lboost_stacktrace_backtrace -lboost_wserialization -lboost_filesystem \
  -lboost_math_tr1f -lboost_stacktrace_basic

#==============================  MACROS  ======================================

GCC_MAJOR = $(shell $(CC) -v 2>&1 | grep " version " | cut -d' ' -f3  | cut -d'.' -f1)
GCC_MINOR = $(shell $(CC) -v 2>&1 | grep " version " | cut -d' ' -f3  | cut -d'.' -f2)

GCC_MIN = 5

# Source files and dependencies
SRCS = $(wildcard $(SRC)/*.cpp)
OBJS = $(patsubst %, $(BIN)/%.o, $(basename $(notdir $(SRCS))))
DEPS = $(patsubst %, $(DEP)/%.d, $(basename $(notdir $(SRCS))))
FREERTOS_SRC = $(wildcard $(FREERTOS_DIR)/*.c)
FREERTOS_SRC += $(wildcard $(FREERTOS_PORT_DIR)/*.c)
FREERTOS_SRC += $(FREERTOS_MEM_DIR)/heap_4.c
FREERTOS_OBJS = $(patsubst %, $(BIN)/%.o, $(basename $(notdir $(FREERTOS_SRC))))
.PRECIOUS: $(FREERTOS_OBJS) $(OBJS) $(ELF)

# External libs
ILIBS = $(BOOST_INC) $(OPENCM3_INC) $(FREERTOS_INC)
LLIBS = $(BOOST_LNK) $(OPENCM3_LNK)

CPP_VER = -std=c++17

# Compile/Link flags
CFLAGS = $(DEBUG) $(ILIBS) $(CPP_VER) $(DEFS) $(WARNINGS) $(FOPT) $(ARCH_FLAGS)
LFLAGS = $(DEBUG) $(LLIBS) $(LDLIBS)

# Dependency flags
DEPFLAGS = -MT $@ -MD -MP -MF $(patsubst %,$(DEP)/%.Td, $(basename $(notdir $<)))

#============================= PHONY RECEPIES =================================
# Build for production
.PHONY: release
release: DEBUG = -Os -DNDEBUG -s
release: $(BINARY)
	@echo Built as Release

.PHONY: debug
debug: DEBUG = -Os -ggdb3
debug: $(BINARY)
	@echo Built as Debug

# Flash
.PHONY: flash
flash: debug
	@echo Flashing $@
	@$(STFLASH) $(FLASHSIZE) write $(BINARY) 0x8000000

# Clean and recompile for production
.PHONY: all
all: clean debug
	@echo Rebuilt from scratch at $(BINARY)

# Clean directories
.PHONY: clean
clean:
	@echo Cleaning temp files
	-@rm -f $(BIN)/*
	-@rm -f $(DEP)/*

#============================ RECEPIES ========================================

# Object files
$(BIN)/%.o: $(SRC)/%.cpp
	@echo Building $@
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -o $@ -c $<
	@mv -f $(DEP)/$*.Td $(DEP)/$*.d

$(BIN)/%.o: $(FREERTOS_DIR)/%.c
	@echo Building $@
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -fpermissive -o $@ -c $< -w
	@mv -f $(DEP)/$*.Td $(DEP)/$*.d

$(BIN)/%.o: $(FREERTOS_PORT_DIR)/%.c
	@echo Building $@
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -fpermissive -o $@ -c $< -w
	@mv -f $(DEP)/$*.Td $(DEP)/$*.d

$(BIN)/%.o: $(FREERTOS_MEM_DIR)/%.c
	@echo Building $@
	@$(CXX) $(DEPFLAGS) $(CFLAGS) -fpermissive -o $@ -c $< -w
	@mv -f $(DEP)/$*.Td $(DEP)/$*.d

# Link
$(BIN)/%.elf: $(OBJS) $(LDSCRIPT) $(FREERTOS_OBJS)
	@echo Linking $@ with $(OBJS) and $(FREERTOS_OBJS)
	@$(LD) $(LDFLAGS) $(OBJS) $(FREERTOS_OBJS) $(LFLAGS) -o $@
	@$(SIZE) $@

$(BIN)/%.bin: $(BIN)/%.elf
	@#printf "  OBJCOPY $(*).bin\n"
	$(OBJCOPY) -Obinary $< $@

$(BIN)/%.hex: $(BIN)/%.elf
	@#printf "  OBJCOPY $(*).hex\n"
	$(OBJCOPY) -Oihex $< $@

$(BIN)/%.srec: $(BIN)/%.elf
	@#printf "  OBJCOPY $(*).srec\n"
	$(OBJCOPY) -Osrec $< $@

$(BIN)/%.list: $(BIN)/%.elf
	@#printf "  OBJDUMP $(*).list\n"
	$(OBJDUMP) -S $< > $@

# Dependencies
.PRECIOUS = $(DEP)/%.d
$(DEP)/%.d: ;

ifeq ($(shell expr $(GCC_MAJOR) \< $(GCC_MIN)), 1)
	$(error GCC is version $(GCC_MAJOR), code in this repo needs version $(GCC_MIN). \
	        Please check `$(CC) --version` for version information. \
	        If you think this is a mistake, please use `make all GCC_MIN=$(GCC_MAJOR)` to rebuild the targets \
	        but be warned, there may be compile errors. There are prebuilt binaries for linux in the bin/ \
	        directory)
endif

# Include Dependencies
-include $(DEPS)
