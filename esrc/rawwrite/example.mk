all: example-rawwrite
.PHONY: example-rawwrite
example-rawwrite: $(BIN_DIR)/rawwrite.com

$(BIN_DIR)/rawwrite.com:	$(TOOLS) $(BIN_DIR)/rawwrite.ihx
	$(LBIN_DIR)/load $(BIN_DIR)/rawwrite

$(BIN_DIR)/rawwrite.ihx:	$(LIBRARIES) $(BIN_DIR)/rawwrite.rel $(BIN_DIR)/rawwrite.arf 
	$(CLD) $(CLD_FLAGS) -nf $(BIN_DIR)/rawwrite.arf

$(BIN_DIR)/rawwrite.rel: $(ESRC_DIR)/rawwrite/rawwrite.c
	$(CCC) $(CCC_FLAGS) -o $(BIN_DIR) $(ESRC_DIR)/rawwrite/rawwrite.c

$(BIN_DIR)/rawwrite.arf:	$(BIN_DIR)/generic.arf
	$(QUIET)$(SED) 's/$(REPLACE_TAG)/rawwrite/' $(BIN_DIR)/generic.arf > $(BIN_DIR)/rawwrite.arf 
