#Define toolchain config
TOOLCHAIN_TARGET := arm-none-eabi-

#Define used JTAG adapter here, included files for different adapters are located in /build/jtag_adapters
JTAG_ADAPTER := stlink_v2
#JTAG_ADAPTER := stm32_serial_loader

#enter global defines here
GLOBAL_DEFINES :=

#default firmware to flash/debug using debug or upload targets
#DEFAULT_FIRMWARE := f4discovery_blinky
#DEFAULT_FIRMWARE := maplemini_blinky
DEFAULT_FIRMWARE := f4discovery_demonstration
#DEFAULT_FIRMWARE := f429idiscovery_blinky

#serialport in case you use the serial loader for flashing
STM32LOADER_SERIALPORT := /dev/ttyUSB0
