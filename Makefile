include common.mk

all: tools libraries examples

clean:	tools-clean libraries-clean examples-clean 
	rm -f $(BIN_DIR)/*

include tools.mk # Local tools
include libraries.mk # Libraries
include examples.mk # Examples

