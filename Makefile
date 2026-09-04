# LeLion GBA — build with devkitARM (libtonc).
# Local builds run inside the official Docker image: `make docker`.
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
# Build inside the official devkitPro image (no local toolchain needed).
.PHONY: docker
docker:
	docker run --rm -v "$(CURDIR):/work" -w /work devkitpro/devkitarm:latest make

ifneq ($(MAKECMDGOALS),docker)
ifeq ($(strip $(DEVKITARM)),)
$(error "Please set DEVKITARM in your environment, or run `make docker`")
endif

include $(DEVKITARM)/gba_rules

TARGET   := lelion
BUILD    := build
SOURCES  := src assets/generated
INCLUDES := src assets/generated
DATA     :=
MUSIC    :=

ARCH     := -mthumb -mthumb-interwork
CFLAGS   := -g -Wall -Wextra -O2 -mcpu=arm7tdmi -mtune=arm7tdmi $(ARCH) -fomit-frame-pointer -ffast-math
CFLAGS   += $(INCLUDE) -DARM7TDMI
CXXFLAGS := $(CFLAGS) -fno-rtti -fno-exceptions
ASFLAGS  := -g $(ARCH)
LDFLAGS   = -g $(ARCH) -Wl,-Map,$(notdir $*.map)

LIBS     := -ltonc
LIBDIRS  := $(LIBTONC) $(LIBGBA)

#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT := $(CURDIR)/$(TARGET)
export VPATH  := $(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) $(foreach dir,$(DATA),$(CURDIR)/$(dir))
export DEPSDIR := $(CURDIR)/$(BUILD)

CFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
SFILES   := $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
BINFILES := $(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

export LD := $(CC)
export OFILES_BIN := $(addsuffix .o,$(BINFILES))
export OFILES_SOURCES := $(CPPFILES:.cpp=.o) $(CFILES:.c=.o) $(SFILES:.s=.o)
export OFILES := $(OFILES_BIN) $(OFILES_SOURCES)
export HFILES := $(addsuffix .h,$(subst .,_,$(BINFILES)))
export INCLUDE := $(foreach dir,$(INCLUDES),-iquote $(CURDIR)/$(dir)) \
                  $(foreach dir,$(LIBDIRS),-I$(dir)/include) -I$(CURDIR)/$(BUILD)
export LIBPATHS := $(foreach dir,$(LIBDIRS),-L$(dir)/lib)

.PHONY: $(BUILD) clean

$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@$(MAKE) --no-print-directory -C $(BUILD) -f $(CURDIR)/Makefile

clean:
	@echo clean ...
	@rm -fr $(BUILD) $(TARGET).elf $(TARGET).gba $(TARGET).map

#---------------------------------------------------------------------------------
else
#---------------------------------------------------------------------------------
DEPENDS := $(OFILES:.o=.d)

$(OUTPUT).gba : $(OUTPUT).elf
$(OUTPUT).elf : $(OFILES)
$(OFILES_SOURCES) : $(HFILES)

%.bin.o %_bin.h : %.bin
	@echo $(notdir $<)
	@$(bin2o)

-include $(DEPENDS)
#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------
endif
