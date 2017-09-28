ROOT := $(PWD)
include $(ROOT)/build/base.mak

FIRMWARE ?= $(DEFAULT_FIRMWARE)

STARTTIME := $(shell date +%s)
# Main targets
all:
	$(call cmd_msg,NOTICE,Build completed in $$(($$(date +%s)-$(STARTTIME))) seconds)

upload: upload-$(FIRMWARE)
debug: debug-$(FIRMWARE) 

SUBDIRS = middlewares libs apps
SELF_DIR = $(abspath $(ROOT))
$(call include-subdirs)

run-openocd-stm32f1x:
	@killall openocd &>/dev/null || openocd -f interface/stlink-v2.cfg -f target/stm32f1x.cfg &>/dev/null &

.PHONY: all clean upload debug
