static const char makefile_data[] =
"# Makefile to rebuild SM64 split image\n"
"\n"
"################ Target Executable and Sources ###############\n"
"\n"
"# BUILD_DIR is location where all build artifacts are placed\n"
"BUILD_DIR = build\n"
"\n"
"##################### Compiler Options #######################\n"
"CROSS = mipsel-elf-\n"
"AS = $(CROSS)as\n"
"CC = $(CROSS)gcc\n"
"LD = $(CROSS)ld\n"
"OBJDUMP = $(CROSS)objdump\n"
"OBJCOPY = $(CROSS)objcopy\n"
"\n"
"ASFLAGS = -EB -mtune=vr4300 -march=vr4300\n"
"CFLAGS  = -Wall -O2 -mtune=vr4300 -march=vr4300 -G 0 -c\n"
"LDFLAGS = -T $(LD_SCRIPT) -Map $(BUILD_DIR)/sm64.map\n"
"\n"
"####################### Other Tools #########################\n"
"\n"
"# N64 tools\n"
"TOOLS_DIR = ../tools\n"
"MIO0TOOL = $(TOOLS_DIR)/mio0\n"
"N64CKSUM = $(TOOLS_DIR)/n64cksum\n"
"N64GRAPHICS = $(TOOLS_DIR)/n64graphics\n"
"EMULATOR = mupen64plus\n"
"EMU_FLAGS = --noosd\n"
"LOADER = loader64\n"
"LOADER_FLAGS = -vwf\n"
"\n"
"FixPath = $(subst /,\\,$1)\n"
"\n"
"######################## Targets #############################\n"
"\n"
"default: all\n"
"\n"
"# file dependencies generated by splitter\n"
"MAKEFILE_SPLIT = Makefile.split\n"
"include $(MAKEFILE_SPLIT)\n"
"\n"
"all: $(TARGET).z64\n"
"\n"
"clean:\n"
"\tdel /Q $(call FixPath,$(BUILD_DIR)/$(TARGET).elf $(BUILD_DIR)/$(TARGET).o $(BUILD_DIR)/$(TARGET).bin $(BUILD_DIR)/$(TARGET).map $(TARGET).z64)\n"
"\n"
"$(MIO0_DIR)/%%.mio0: $(MIO0_DIR)/%%.bin\n"
"\t$(MIO0TOOL) $< $@\n"
"\n"
"$(BUILD_DIR):\n"
"\tmkdir $(BUILD_DIR)\n"
"\n"
"$(BUILD_DIR)/$(TARGET).o: $(TARGET).s Makefile $(MAKEFILE_SPLIT) $(MIO0_FILES) $(LEVEL_FILES) $(MUSIC_FILES) | $(BUILD_DIR)\n"
"\t$(AS) $(ASFLAGS) -o $@ $<\n"
"\n"
"$(BUILD_DIR)/%%.o: %%.c Makefile.as | $(BUILD_DIR)\n"
"\t$(CC) $(CFLAGS) -o $@ $<\n"
"\n"
"$(BUILD_DIR)/$(TARGET).elf: $(BUILD_DIR)/$(TARGET).o $(LD_SCRIPT)\n"
"\t$(LD) $(LDFLAGS) -o $@ $< $(LIBS)\n"
"\n"
"$(BUILD_DIR)/$(TARGET).bin: $(BUILD_DIR)/$(TARGET).elf\n"
"\t$(OBJCOPY) $< $@ -O binary\n"
"\n"
"# final z64 updates checksum\n"
"$(TARGET).z64: $(BUILD_DIR)/$(TARGET).bin\n"
"\t$(N64CKSUM) $< $@\n"
"\n"
"$(BUILD_DIR)/$(TARGET).hex: $(TARGET).z64\n"
"\txxd $< > $@\n"
"\n"
"$(BUILD_DIR)/$(TARGET).objdump: $(BUILD_DIR)/$(TARGET).elf\n"
"\t$(OBJDUMP) -D $< > $@\n"
"\n"
"test: $(TARGET).z64\n"
"\t$(EMULATOR) $(EMU_FLAGS) $<\n"
"\n"
"load: $(TARGET).z64\n"
"\t$(LOADER) $(LOADER_FLAGS) $<\n"
"\n"
".PHONY: all clean default diff test\n"
;
