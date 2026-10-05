CL65 ?= cl65
CC65_HOME ?= $(shell dirname $$(dirname $$(readlink -f $$(command -v $(CL65)))))/share/cc65
BUILD := build
SOURCES := src/main.c src/video.c src/hardware.c src/lessons.c src/timing.s

.PHONY: all clean disk check
all: $(BUILD)/FELIX

$(BUILD):
	mkdir -p $(BUILD)

# The stock HGR layout reserves $2000-$3FFF and puts C code at $4000.
# apple2 (6502), not apple2enh (65C02), supports unenhanced IIe machines.
$(BUILD)/FELIX: $(SOURCES) src/hardware.h src/video.h src/lessons.h | $(BUILD)
	$(CL65) -t apple2 -C apple2-hgr.cfg -Oirs -I src \
		-m $(BUILD)/felix.map -Ln $(BUILD)/felix.lbl -o $@ $(SOURCES)
	rm -f src/*.o

disk: $(BUILD)/FELIX
	node tools/make-disk.mjs $(BUILD)/FELIX "$(CC65_HOME)/target/apple2/util/loader.system" $(BUILD)/felix.po vendor/ProDOS_2_4_3.po

check: disk
	node tools/verify-build.mjs
	node tools/verify-keyboard.mjs

clean:
	rm -rf $(BUILD)
	rm -f src/*.o