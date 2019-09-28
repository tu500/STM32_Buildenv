# Target file name.
TARGET = bluepill_enc28j60

# List C source files here.
CCSOURCES = main.c interrupts.c hardware.c enc28j60.c
CCSOURCES += uip_interface.c 
CCSOURCES += clock-arch.c 

#CCSOURCES += uip/psock.c
CCSOURCES += uip/timer.c
CCSOURCES += uip/uip_arp.c
CCSOURCES += uip/uip.c
#CCSOURCES += uip/uip-fw.c
#CCSOURCES += uip/uiplib.c
#CCSOURCES += uip/uip-neighbor.c
#CCSOURCES += uip/uip-split.c

CCSOURCES += mqtt.c
CCSOURCES += autoc4.c

CCSOURCES += ws2812b.c

# List C++ source files here.
CXXSOURCES =

# List Files to be assembled here
ASOURCES = 

# C compiler flags
CFLAGS  = -std=gnu99 -ggdb -O0 -Werror -mthumb -mcpu=cortex-m3
CFLAGS += -fdata-sections -ffunction-sections

# C++ compiler flags
CXXFLAGS =

# GAS flags
ASFLAGS = -fdata-sections -ffunction-sections

# Linker flags
LDFLAGS := -lc -Wl,--gc-sections
# LDFLAGS += -specs=rdimon.specs
LDFLAGS += -specs=nosys.specs
LDFLAGS += -mthumb -mcpu=cortex-m3
LDFLAGS += -Wl,-T$(ROOT)/misc/linker/f1/STM32F103XB_FLASH.ld,-Map,$(SELF_DIR)/$(TARGET).map

# Additional include paths to consider
INCLUDES = $(SELF_DIR)/inc
INCLUDES += $(SELF_DIR)/src/uip

# Middlewares to add
# source files are built with this target (specific for this target, so local header files are consideres)
# include paths are added automatically
MIDDLEWARES = cmsis stm32f1_device_103xb stm32f1_hal

# Additional local static libs to link against
LIBS =

# Folder for object files
OBJDIR = obj

# Folder for sourcecode
SRCDIR = src

# Additional defines
DEFINES := -DSTM32F103xB -DUSE_HAL_DRIVER

include $(ROOT)/build/targets/executable.mak
