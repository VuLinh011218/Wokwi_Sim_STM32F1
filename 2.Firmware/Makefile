##############################################################################
# Makefile for the STM32F103C8T6 Wokwi Demo firmware project
#
# Usage:
#   make            # -> build/Demo.elf/.hex/.bin/.map
#   make clean      # remove build output
##############################################################################

TARGET    = Demo
BUILD_DIR = build

######################################
# Toolchain (auto-detects STM32CubeIDE toolchain if not already on PATH)
######################################
CUBEIDE_GCC_DIR := C:/ST/STM32CubeIDE_1.19.0/STM32CubeIDE/plugins/com.st.stm32cube.ide.mcu.externaltools.gnu-tools-for-stm32.13.3.rel1.win32_1.0.0.202411081344/tools/bin
ifneq ($(wildcard $(CUBEIDE_GCC_DIR)/arm-none-eabi-gcc.exe),)
PREFIX ?= $(CUBEIDE_GCC_DIR)/arm-none-eabi-
else
PREFIX ?= arm-none-eabi-
endif

CC     = "$(PREFIX)gcc"
AS     = "$(PREFIX)gcc" -x assembler-with-cpp
CP     = "$(PREFIX)objcopy"
SZ     = "$(PREFIX)size"
HEX    = $(CP) -O ihex
BIN    = $(CP) -O binary -S

######################################
# MCU
######################################
MCU = -mcpu=cortex-m3 -mthumb -mfloat-abi=soft

######################################
# Defines
######################################
C_DEFS = \
-DSTM32F103xB \
-DUSE_FULL_LL_DRIVER

######################################
# Project layout
######################################
PLATFORM_DIR = Platform/Demo

######################################
# Recursive wildcard helper (pure GNU Make, no shell dependency)
######################################
rwildcard = $(foreach d,$(wildcard $1*),$(call rwildcard,$d/,$2) $(filter $(subst *,%,$2),$d))

######################################
# Sources
######################################
ASM_SOURCES = $(PLATFORM_DIR)/Core/Startup/startup_stm32f103c8tx.s

C_SOURCES_CORE = $(wildcard $(PLATFORM_DIR)/Core/Src/*.c)
C_SOURCES_HAL  = $(wildcard $(PLATFORM_DIR)/Drivers/STM32F1xx_HAL_Driver/Src/*.c)

DEV_EXCLUDE_DIRS = \
Dev/3.Drivers/Components/SPI/samv71/% \
Dev/3.Drivers/Components/dio/nxp/% \
Dev/3.Drivers/Components/dio/samv71/% \
Dev/3.Drivers/Components/dio/dummy/%

C_SOURCES_DEV  = $(filter-out $(DEV_EXCLUDE_DIRS),$(call rwildcard,Dev/,*.c))

C_SOURCES = $(C_SOURCES_CORE) $(C_SOURCES_HAL) $(C_SOURCES_DEV)

######################################
# Includes
######################################
C_INCLUDES = \
-I$(PLATFORM_DIR)/Core/Inc \
-I$(PLATFORM_DIR)/Drivers/STM32F1xx_HAL_Driver/Inc \
-I$(PLATFORM_DIR)/Drivers/STM32F1xx_HAL_Driver/Inc/Legacy \
-I$(PLATFORM_DIR)/Drivers/CMSIS/Device/ST/STM32F1xx/Include \
-I$(PLATFORM_DIR)/Drivers/CMSIS/Include

DEV_INC_DIRS = $(sort $(dir $(call rwildcard,Dev/,*.h)))
C_INCLUDES  += $(addprefix -I,$(DEV_INC_DIRS))

######################################
# Compile flags
######################################
OPT ?= -Og

CFLAGS  = $(MCU) $(C_DEFS) $(C_INCLUDES) $(OPT) -Wall -fdata-sections -ffunction-sections -g -gdwarf-2
CFLAGS += -MMD -MP -MF"$(@:%.o=%.d)"

ASFLAGS = $(MCU) -g -gdwarf-2

######################################
# Link flags
######################################
LDSCRIPT = $(PLATFORM_DIR)/STM32F103C8TX_FLASH.ld
LIBS     = -lc -lm -lnosys
LDFLAGS  = $(MCU) -specs=nano.specs -T$(LDSCRIPT) $(LIBS) \
           -Wl,-Map=$(BUILD_DIR)/$(TARGET).map,--cref -Wl,--gc-sections

######################################
# Portable mkdir/rm
######################################
ifeq ($(OS),Windows_NT)
SHELL     := cmd.exe
MKDIR_CMD = if not exist "$(BUILD_DIR)" mkdir "$(BUILD_DIR)"
RM_CMD    = if exist "$(BUILD_DIR)" rmdir /S /Q "$(BUILD_DIR)"
else
MKDIR_CMD = mkdir -p $(BUILD_DIR)
RM_CMD    = rm -rf $(BUILD_DIR)
endif

######################################
# Build rules
######################################
OBJECTS  = $(addprefix $(BUILD_DIR)/,$(notdir $(C_SOURCES:.c=.o)))
OBJECTS += $(addprefix $(BUILD_DIR)/,$(notdir $(ASM_SOURCES:.s=.o)))

vpath %.c $(sort $(dir $(C_SOURCES)))
vpath %.s $(sort $(dir $(ASM_SOURCES)))

.PHONY: all clean size

all: $(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).hex $(BUILD_DIR)/$(TARGET).bin size

$(BUILD_DIR)/%.o: %.c Makefile | $(BUILD_DIR)
	$(CC) -c $(CFLAGS) -o $@ $<

$(BUILD_DIR)/%.o: %.s Makefile | $(BUILD_DIR)
	$(AS) -c $(ASFLAGS) -o $@ $<

$(BUILD_DIR)/$(TARGET).elf: $(OBJECTS) $(LDSCRIPT)
	$(CC) $(OBJECTS) $(LDFLAGS) -o $@

$(BUILD_DIR)/%.hex: $(BUILD_DIR)/%.elf
	$(HEX) $< $@

$(BUILD_DIR)/%.bin: $(BUILD_DIR)/%.elf
	$(BIN) $< $@

size: $(BUILD_DIR)/$(TARGET).elf
	$(SZ) $<

$(BUILD_DIR):
	$(MKDIR_CMD)

clean:
	$(RM_CMD)

-include $(wildcard $(BUILD_DIR)/*.d)
