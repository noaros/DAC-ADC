TARGET  := dac_adc
BUILD   := build
SRCS    := src/main.c src/startup.c
LDSCRIPT:= src/stm32f429zi.ld

CC      := arm-none-eabi-gcc
OBJCOPY := arm-none-eabi-objcopy
SIZE    := arm-none-eabi-size

CFLAGS  := -mcpu=cortex-m4 -mthumb -Os -g -std=c11 -Wall -Wextra \
           -ffunction-sections -fdata-sections
LDFLAGS := -T$(LDSCRIPT) -nostdlib -Wl,--gc-sections -Wl,--no-warn-rwx-segments -Wl,-Map=$(BUILD)/$(TARGET).map

all: $(BUILD)/$(TARGET).bin

$(BUILD)/$(TARGET).elf: $(SRCS) $(LDSCRIPT) | $(BUILD)
	$(CC) $(CFLAGS) $(SRCS) $(LDFLAGS) -o $@
	$(SIZE) $@

$(BUILD)/$(TARGET).bin: $(BUILD)/$(TARGET).elf
	$(OBJCOPY) -O binary $< $@

$(BUILD):
	mkdir -p $@

flash: $(BUILD)/$(TARGET).elf
	STM32_Programmer_CLI -c port=SWD -w $< -v -rst

clean:
	rm -rf $(BUILD)

.PHONY: all flash clean
