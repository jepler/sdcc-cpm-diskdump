include common.mk

all: tools libraries examples \
	example-diskdump

clean:	tools-clean libraries-clean examples-clean 
	rm -f $(BIN_DIR)/*

include tools.mk # Local tools
include libraries.mk # Libraries
include examples.mk # Examples

include esrc/diskdump/diskdump.mk

