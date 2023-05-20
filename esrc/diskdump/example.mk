all: example-diskdump
.PHONY: example-diskdump
example-diskdump: $(BIN_DIR)/diskdump.com

$(BIN_DIR)/diskdump.com:	$(TOOLS) $(BIN_DIR)/diskdump.ihx
	$(LBIN_DIR)/load $(BIN_DIR)/diskdump

$(BIN_DIR)/diskdump.ihx:	$(LIBRARIES) $(BIN_DIR)/diskdump.rel $(BIN_DIR)/diskdump.arf 
	$(CLD) $(CLD_FLAGS) -nf $(BIN_DIR)/diskdump.arf

$(BIN_DIR)/diskdump.rel: $(ESRC_DIR)/diskdump/diskdump.c
	$(CCC) $(CCC_FLAGS) -o $(BIN_DIR) $(ESRC_DIR)/diskdump/diskdump.c

$(BIN_DIR)/diskdump.arf:	$(BIN_DIR)/generic.arf
	$(QUIET)$(SED) 's/$(REPLACE_TAG)/diskdump/' $(BIN_DIR)/generic.arf > $(BIN_DIR)/diskdump.arf 
