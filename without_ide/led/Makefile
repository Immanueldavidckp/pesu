# Makefile - STM32F767ZI bare metal
#
#   make APP=led          build the blink example  (default)
#   make APP=pwm          build the TIM1_CH1 PWM example
#   make APP=pwm flash    build + flash with st-flash
#   make openocd-flash    flash with OpenOCD instead
#   make clean

APP    ?= led
TARGET  = $(APP)
BUILD   = build

PREFIX  = arm-none-eabi-
CC      = $(PREFIX)gcc
OBJCOPY = $(PREFIX)objcopy
OBJDUMP = $(PREFIX)objdump
SIZE    = $(PREFIX)size

# Cortex-M7 with the double-precision FPU that the F767 actually has
CPU = -mcpu=cortex-m7 -mthumb -mfpu=fpv5-d16 -mfloat-abi=hard

CFLAGS  = $(CPU) -std=c11 -O2 -g3 \
          -Wall -Wextra -Wshadow -Wundef \
          -ffunction-sections -fdata-sections -fno-common \
          -fno-builtin -I.

LDFLAGS = $(CPU) -T linker.ld -nostartfiles \
          -Wl,--gc-sections -Wl,-Map=$(BUILD)/$(TARGET).map,--cref

SRCS = startup.c main_$(APP).c
OBJS = $(addprefix $(BUILD)/,$(SRCS:.c=.o))
DEPS = $(OBJS:.o=.d)

.PHONY: all clean flash openocd-flash size disasm

all: $(BUILD)/$(TARGET).elf $(BUILD)/$(TARGET).bin $(BUILD)/$(TARGET).hex

$(BUILD):
	mkdir -p $(BUILD)

$(BUILD)/%.o: %.c | $(BUILD)
	$(CC) $(CFLAGS) -MMD -MP -c $< -o $@

$(BUILD)/$(TARGET).elf: $(OBJS) linker.ld
	$(CC) $(OBJS) $(LDFLAGS) -o $@
	@echo
	$(SIZE) $@

$(BUILD)/%.bin: $(BUILD)/%.elf
	$(OBJCOPY) -O binary $< $@

$(BUILD)/%.hex: $(BUILD)/%.elf
	$(OBJCOPY) -O ihex $< $@

# --- flashing ---------------------------------------------------------------
# stlink tools
flash: $(BUILD)/$(TARGET).bin
	st-flash --reset write $< 0x08000000

# OpenOCD (works with the on-board ST-LINK of the Nucleo-144)
openocd-flash: $(BUILD)/$(TARGET).elf
	openocd -f interface/stlink.cfg -f target/stm32f7x.cfg \
	        -c "program $< verify reset exit"

# --- helpers ----------------------------------------------------------------
size: $(BUILD)/$(TARGET).elf
	$(SIZE) -A $<

disasm: $(BUILD)/$(TARGET).elf
	$(OBJDUMP) -d -S $< > $(BUILD)/$(TARGET).lst
	@echo "wrote $(BUILD)/$(TARGET).lst"

clean:
	rm -rf $(BUILD)

-include $(DEPS)
