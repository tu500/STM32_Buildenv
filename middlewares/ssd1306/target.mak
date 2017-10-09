TARGET = ssd1306

# List C source files here.
CCSOURCES = fonts.c ssd1306.c

# List C++ source files here.
CXXSOURCES =

# List Files to be assembled here
ASOURCES =

# Additional include paths to consider
INCLUDES = $(ROOT)/middlewares/ssd1306/Inc

# Folder for sourcecode
SRCDIR = Src

include $(ROOT)/build/targets/middleware.mak
