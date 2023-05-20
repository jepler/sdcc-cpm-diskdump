include common.mk

.PHONY: all
all: examples

.PHONY: clean
clean:	tools-clean libraries-clean examples-clean 
	rm -f $(BIN_DIR)/*

include tools.mk # Local tools
include libraries.mk # Libraries
include examples.mk # Examples

$(foreach project-mk,$(wildcard esrc/*/example.mk),$(eval include $(project-mk)))

