BUILD_BASE	= build
TARGET_BASE	= lib

# Base directory for the compiler
XTENSA_TOOLS_ROOT ?= c:/Espressif/xtensa-lx106-elf/bin

# base directory of the ESP8266 SDK package, absolute
SDK_BASE	?= c:/Espressif/ESP8266_SDK

# which modules (subdirectories) of the project to include in compiling
MODULES		= config driver mqtt yxml

# compiler flags using during compilation of source files
CFLAGS		= -Os -g -O2 -std=gnu90 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -mno-serialize-volatile -D__ets__ -DICACHE_FLASH -DESP8266_SDK
CPPFLAGS	= -Os -g -O2 -Wpointer-arith -Wundef -Werror -Wl,-EL -fno-inline-functions -nostdlib -mlongcalls -mtext-section-literals -mno-serialize-volatile -D__ets__ -DICACHE_FLASH -DESP8266_SDK

# various paths from the SDK used in this project
SDK_INCDIR	= include include/json
EXTRA_INCDIR = include $(SDK_BASE)/../extra/include

# select which tools to use as compiler, librarian and linker
CC	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-gcc
CPP	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-g++
AR	:= $(XTENSA_TOOLS_ROOT)/xtensa-lx106-elf-ar

# no user configurable options below here
SRC_DIR		:= $(MODULES)
BUILD_DIR	:= $(addprefix $(BUILD_BASE)/,$(MODULES))

SRC			:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.c))
SRC_CPP		:= $(foreach sdir,$(SRC_DIR),$(wildcard $(sdir)/*.cpp))

TARGET_OUT	:= $(addsuffix .a,$(addprefix $(TARGET_BASE)/lib,$(MODULES)))

INCDIR			:= $(addprefix -I,$(SRC_DIR))
SDK_INCDIR		:= $(addprefix -I$(SDK_BASE)/,$(SDK_INCDIR))
EXTRA_INCDIR	:= $(addprefix -I,$(EXTRA_INCDIR))
MODULE_INCDIR	:= $(addsuffix /include,$(INCDIR))

V ?= $(VERBOSE)
ifeq ("$(V)","1")
Q :=
vecho := @true
else
Q := @
vecho := @echo
endif

vpath %.c $(SRC_DIR)
vpath %.cpp $(SRC_DIR)

define compile-objects
$1/%.o: %.c
	$(vecho) "CC $$<"
	$(Q) $(CC) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CFLAGS)  -c $$< -o $$@

$1/%.o: %.cpp
	$(vecho) "CC $$<"
	$(Q) $(CPP) $(INCDIR) $(MODULE_INCDIR) $(EXTRA_INCDIR) $(SDK_INCDIR) $(CPPFLAGS)  -c $$< -o $$@
endef

define make-libraries
$(addprefix $(TARGET_BASE)/lib,$1.a): $(patsubst %.c,$(BUILD_BASE)/%.o,$(wildcard $1/*.c)) $(patsubst %.cpp,$(BUILD_BASE)/%.o,$(wildcard $1/*.cpp))
	$(vecho) "AR $$@"
	$(Q) $(AR) cru $$@ $$^
endef

.PHONY: all checkdirs clean rebuild

all: checkdirs $(TARGET_OUT)

checkdirs: $(BUILD_DIR) $(TARGET_BASE)

$(BUILD_DIR):
	$(Q) mkdir -p $@

$(TARGET_BASE):
	$(Q) mkdir -p $@

rebuild: clean all

clean:
	$(Q) rm -f $(TARGET_OUT)
	$(Q) rm -rf $(BUILD_DIR)
	$(Q) rm -rf $(BUILD_BASE)
	$(Q) rm -rf $(TARGET_BASE)

$(foreach bdir,$(BUILD_DIR),$(eval $(call compile-objects,$(bdir))))
$(foreach lib,$(MODULES),$(eval $(call make-libraries,$(lib))))
