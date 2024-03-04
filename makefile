SHELL := bash
.ONESHELL:

TOP     := top
TB_MAIN := tb

SRC     := src
RSC     := rsc
TB      := tb
BUILD   := build
RUNS    := runs
SEED    := 10
DEVICE  := up5k
PIN_DEF := $(RSC)/tt3_asic_sim.pcf
PACKAGE := sg48
FREQ    := 10

USB_VENDOR  ?= 1d50
USB_PRODUCT ?= 6146

SOURCES     := $(BUILD)/top.sv
TLV_SOURCES := $(shell find $(SRC) -type f -name '*.tlv' -and -not -name 'top.tlv')
TB_SOURCES  := $(shell fd '.*\.cpp' $(TB))

CXXFLAGS += -std=c++23

ifndef VERBOSE
.SILENT:
SILENT_SUBMAKE := -MAKEFLAGS --silent
endif

.PHONY: build prog clean gds
.PRECIOUS: $(BUILD)/%.json $(BUILD)/%.asc $(BUILD)/%.log $(BUILD)/%.sv

gen: $(SOURCES)

build: $(BUILD)/$(TOP).bin

prog_ice: $(BUILD)/$(TOP).bin
	iceprog $<

prog: $(BUILD)/$(TOP).bin
	dfu-util -R -d $(USB_VENDOR):$(USB_PRODUCT) -a 0 -D $<

clean:
	rm -rf $(BUILD) runs

$(BUILD)/:
	mkdir -p $(BUILD)
	echo '*' > $(BUILD)/.gitignore

$(BUILD)/%/: | $(BUILD)/
	mkdir -p $@

$(BUILD)/%.json: $(SOURCES) | $(BUILD)/
	yosys \
		-l $(BUILD)/yosys.log \
		-DSYNTH \
		-p 'synth_ice40 -top $(basename $(notdir $@)) -json $@' \
		$^

$(BUILD)/%.asc: $(BUILD)/%.json | $(BUILD)/
	nextpnr-ice40 \
		-l $(BUILD)/nextpnr.log \
		--pcf-allow-unconstrained \
		--seed $(SEED) \
		--freq $(FREQ) \
		--package $(PACKAGE) \
		--$(DEVICE) \
		--asc $@ \
		--pcf $(PIN_DEF) \
		--json $<

$(BUILD)/%.bin: $(BUILD)/%.asc | $(BUILD)/
	icepack $< $@

$(BUILD)/top.sv: $(SRC)/top.tlv $(TLV_SOURCES) | $(BUILD)/
	cd $(BUILD) && sandpiper-saas --iArgs -i ../$< -o $(notdir $@) $(if $(TLV_SOURCES),-f $(TLV_SOURCES:%=../%))

PDK := sky130A

# 1x1: "0 0 161.00 111.52"
# 1x2: "0 0 161.00 225.76"
# 2x2: "0 0 334.88 225.76"
# 3x2: "0 0 508.76 225.76"
# 4x2: "0 0 682.64 225.76"
# 6x2: "0 0 1030.40 225.76"
# 8x2: "0 0 1378.16 225.76"
$(BUILD)/user_config.tcl: | $(BUILD)/
	cat <<- EOF > $@
		set ::env(DESIGN_NAME) "$(TOP)"
		set ::env(VERILOG_FILES) "/work/$(BUILD)/top.sv"
		set ::env(DIE_AREA) "0 0 508.76 225.76"
		set ::env(FP_DIE_TEMPLATE) "$$DESIGN_DIR/../rsc/3x2_pg.def"
	EOF

$(BUILD)/config.tcl: $(RSC)/config.tcl | $(BUILD)/
	cp $< $@

gds: runs/latest/results/final/gds/$(TOP).gds

runs/latest/results/final/gds/$(TOP).gds: $(BUILD)/user_config.tcl $(BUILD)/config.tcl $(SOURCES)
	docker run --rm \
		-v $(OPENLANE_ROOT):/openlane \
		-v $(PDK_ROOT):/pdk \
		-v $(PWD):/work \
		-e PDK=$(PDK) \
		-e PDK_ROOT=/pdk \
		-u `id -u $$USER`:`id -g $$USER` \
		$(OPENLANE_IMAGE_NAME) \
		/bin/bash -c \
		"./flow.tcl -overwrite -design /work/$(BUILD) -run_path /work/$(RUNS) -tag latest"

