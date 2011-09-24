#---------------------------------------------------------------------------------
# Clear the implicit built in rules
#---------------------------------------------------------------------------------
.SUFFIXES:
#---------------------------------------------------------------------------------
ifeq ($(strip $(PSL1GHT)),)
$(error "Please set PSL1GHT in your environment. export PSL1GHT=<path>")
endif

include $(PSL1GHT)/ppu_rules

# NO edits needed below here
 
APPEGO		:=	PS3Roar
APPNAME	:=	PS3Roar
TITLE		:=	$(APPNAME)
APPVERSION	:=	003
APPID		:=	$(APPNAME)$(APPVERSION)
APPRAND	:=	$(shell openssl rand -hex 8 | tr [:lower:] [:upper:])
CONTENTID	:=	$(APPEGO)-$(APPID)_00-$(APPRAND)
SFOXML		:=	pkg/SFO.XML
ICON0		:=	pkg/ICON0.PNG
PIC1		:=	pkg/extras/PIC1.PNG

#---------------------------------------------------------------------------------
# TARGET is the name of the output
# BUILD is the directory where object files & intermediate files will be placed
# SOURCES is a list of directories containing source code
# INCLUDES is a list of directories containing extra header files
#---------------------------------------------------------------------------------
TARGET		:=	$(notdir $(CURDIR))
BUILD		:=	build
SOURCES	:=	source
DATA		:=	data
SHADERS	:=	shaders
INCLUDES	:=	include
PAYLOADS	:=	payloads

#---------------------------------------------------------------------------------
# options for code generation
#---------------------------------------------------------------------------------
CFLAGS		=	-g -O3 -Wall --std=gnu99 -mcpu=cell $(MACHDEP) $(INCLUDE) -maltivec -mminimal-toc
CXXFLAGS	=	$(CFLAGS)
LDFLAGS	=	$(MACHDEP) -Wl,-Map,$(notdir $@).map

#---------------------------------------------------------------------------------
# any extra libraries we wish to link with the project
#---------------------------------------------------------------------------------
LIBS		:=	-lSDL_gfx -lSDL_ttf -lSDL_mixer -lSDL -lpngdec -ljpgdec -lSDL_image -lSDLmain -ltiff -lpng -ljpeg -lSDL_ttf -lSDL_gfx -lvorbisfile -lvorbis -logg -laudio -lmikmod -lrsx -lgcm_sys -lsysutil -lsysmodule -lio -lfreetype -lcairo -lfreetype -lz -lpixman-1 -lrt -llv2 -lm


#---------------------------------------------------------------------------------
# list of directories containing libraries, this must be the top level containing
# include and lib
#---------------------------------------------------------------------------------
LIBDIRS		:=	$(PS3DEV)/portlibs/ppu $(PSL1GHT)/ppu $(PSL1GHT)/ppu/lib

#---------------------------------------------------------------------------------
# no real need to edit anything past this point unless you need to add additional
# rules for different file extensions
#---------------------------------------------------------------------------------
ifneq ($(BUILD),$(notdir $(CURDIR)))
#---------------------------------------------------------------------------------
export OUTPUT		:=	$(CURDIR)/$(TARGET)
export VPATH		:=	$(foreach dir,$(SOURCES),$(CURDIR)/$(dir)) \
				$(foreach dir,$(DATA),$(CURDIR)/$(dir)) \
				$(foreach dir,$(SHADERS),$(CURDIR)/$(dir))
export DEPSDIR	:=	$(CURDIR)/$(BUILD)
export BUILDDIR	:=	$(CURDIR)/$(BUILD)

export PKGFILES	:=	$(CURDIR)/pkg/extras

#---------------------------------------------------------------------------------
# automatically build a list of object files for our project
#---------------------------------------------------------------------------------
CFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
CPPFILES	:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
sFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.s)))
SFILES		:=	$(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.S)))
BINFILES	:=	$(foreach dir,$(DATA),$(notdir $(wildcard $(dir)/*.*)))

#---------------------------------------------------------------------------------
# use CXX for linking C++ projects, CC for standard C
#---------------------------------------------------------------------------------
ifeq ($(strip $(CPPFILES)),)
	export LD	:=	$(CC)
else
	export LD	:=	$(CXX)
endif

export OFILES	:=	$(addsuffix .o,$(BINFILES)) \
			$(CFILES:.c=.o) \
			$(CPPFILES:.cpp=.o) \
			$(sFILES:.s=.o) $(SFILES:.S=.o) \
			$(addsuffix .o,$(VCGFILES)) \
			$(addsuffix .o,$(FCGFILES)) \
			$(addsuffix .o,$(VPOFILES)) \
			$(addsuffix .o,$(FPOFILES))

#---------------------------------------------------------------------------------
# build a list of include paths
#---------------------------------------------------------------------------------
export INCLUDE	:=	$(foreach dir,$(INCLUDES), -I$(CURDIR)/$(dir)) \
			$(foreach dir,$(LIBDIRS),-I$(dir)/include) \
			$(LIBPSL1GHT_INC) \
			-I$(CURDIR)/$(BUILD)\
			-I$(PS3DEV)/portlibs/ppu/include

#---------------------------------------------------------------------------------
# build a list of library paths
#---------------------------------------------------------------------------------
export LIBPATHS	:=	$(foreach dir,$(LIBDIRS),-L$(dir)/lib) \
			$(LIBPSL1GHT_LIB) -L $(PSL1GHT)/ppu/lib -L$(PS3DEV)/portlibs/ppu/lib
export OUTPUT	:=	$(CURDIR)/$(TARGET)

#---------------------------------------------------------------------------------
.PHONY: $(BUILD) clean pkg run

#---------------------------------------------------------------------------------
$(BUILD):
	@[ -d $@ ] || mkdir -p $@
	@echo compiling PS3Roar
	@$(MAKE) --no-print-directory -C $(BUILDDIR) -f $(CURDIR)/Makefile

#---------------------------------------------------------------------------------
clean:
	@echo clean ...
#	#@[ -d PL3 ] && $(MAKE) --no-print-directory -C clean > /dev/null
	@rm -rf $(BUILD) $(OUTPUT).elf $(OUTPUT).self $(OUTPUT).a $(OUTPUT)*.pkg

#---------------------------------------------------------------------------------
run:
	ps3load $(OUTPUT).self

#---------------------------------------------------------------------------------
pkg:	self $(OUTPUT).pkg

#---------------------------------------------------------------------------------
self:	$(BUILD) $(OUTPUT).self

#---------------------------------------------------------------------------------
dex:	$(BUILD) $(OUTPUT).elf
	$(VERB) echo creating DEX fself
	$(VERB) cp $(OUTPUT).elf $(BUILDDIR)/$(notdir $(OUTPUT).elf)
	$(VERB) $(SPRX) $(BUILDDIR)/$(notdir $(OUTPUT).elf)
	$(VERB) make_fself $(BUILDDIR)/$(notdir $(OUTPUT).elf) $(OUTPUT).dex.self

#---------------------------------------------------------------------------------
else

DEPENDS		:=	$(OFILES:.o=.d)

#---------------------------------------------------------------------------------
# main targets
#---------------------------------------------------------------------------------
#$(OUTPUT).pkg:	$(OUTPUT).self
$(OUTPUT).self: $(OUTPUT).elf
$(OUTPUT).elf:	$(OFILES)

#---------------------------------------------------------------------------------
# This rule links in binary data with the .bin extension
#---------------------------------------------------------------------------------
%.bin.o	:	%.bin
#---------------------------------------------------------------------------------
	@echo $(notdir $<)
	@$(bin2o)

#---------------------------------------------------------------------------------
-include $(DEPENDS)

#---------------------------------------------------------------------------------
endif
#---------------------------------------------------------------------------------

