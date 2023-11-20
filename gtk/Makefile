comma := ,
empty :=
space := $(empty) $(empty)
# Directories
TARGET	:= bin
BUILD	:= obj
SOURCES	:= src
LIBS	:= lib
GENASM	:= genasm
# Find source files
export CFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.c)))
export FULLCFILES	:= $(addprefix $(SOURCES)/,$(CFILES))
export CPPFILES	:= $(foreach dir,$(SOURCES),$(notdir $(wildcard $(dir)/*.cpp)))
export FULLCPPFILES	:= $(addprefix $(SOURCES)/,$(CPPFILES))
export OFILES	:= $(CFILES:.c=.o) $(CPPFILES:.cpp=.o)
export FULLOFILES	:= $(addprefix $(BUILD)/,$(OFILES))
export GENASMFILESC	:= $(addprefix $(GENASM)/,$(CFILES:.c=.asm))
export GENASMFILESCPP	:= $(addprefix $(GENASM)/,$(CPPFILES:.cpp=.asm))
# Common flags
export CFLAGS := -fopenmp -idirafter ./src/include `pkg-config gtkmm-3.0 --cflags`
export CXXFLAGS	:= $(CFLAGS) -DGLIBMM_DISABLE_DEPRECATED -DGIOMM_DISABLE_DEPRECATED -DGTKMM_DISABLE_DEPRECATED -DGDKMM_DISABLE_DEPRECATED
export DEBUGFLAGS := -Og
export RELFLAGS := -O3
export LINKFLAGS := -L $(LIBS) -O -s --gc-sections
export LINKLIBS := avcodec avformat avutil swresample swscale
export LINKS := $(addprefix -l,$(LINKLIBS))
export GCCLINKS := -Wl,$(subst $(space),$(comma),$(strip $(LINKFLAGS))) $(LINKS) `pkg-config gtkmm-3.0 --libs`

.PHONY : showasm

all : $(TARGET)/98videoconverter $(BUILD) $(TARGET)
	
$(BUILD):
	mkdir -p $@

$(TARGET):
	mkdir -p $@
	
$(TARGET)/98videoconverter : $(FULLOFILES)
	g++ -o $(TARGET)/98videoconverter $^ $(CXXFLAGS) $(GCCLINKS)

$(BUILD)/%.o : $(SOURCES)/%.c
	gcc -o $@ $< -c $(RELFLAGS) $(CFLAGS)

$(BUILD)/%.o : $(SOURCES)/%.cpp
	g++ -o $@ $< -c $(RELFLAGS) $(CXXFLAGS)

showasm	: $(GENASMFILESC) $(GENASMFILESCPP)

$(GENASM)/%.asm	: $(SOURCES)/%.c
	gcc $< -S -o $@ -c $(RELFLAGS) $(CFLAGS)

$(GENASM)/%.asm	: $(SOURCES)/%.cpp
	g++ $< -S -o $@ -c $(RELFLAGS) $(CXXFLAGS)