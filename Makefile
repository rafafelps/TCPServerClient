WARNINGS = -Wall
OPTIMS = -O2
DBGFLAGS = -g -Og
DMACROS =
INCDIRS = -Iinclude $(addprefix -I,$(strip $(shell dir include /ad-h /b /s 2>nul))) -Isrc $(addprefix -I,$(strip $(shell dir src /ad-h /b /s 2>nul)))
CSRC = $(shell dir /s/b *.c 2> nul)
CPPSRC = $(shell dir /s/b *.cpp 2> nul)
OBJS = $(addprefix obj\,$(filter-out $(shell cd)\,$(subst src\, src\,$(addsuffix .o,$(CSRC) $(CPPSRC)))))
LIBDIRS = $(addprefix -L,$(strip $(shell dir lib /ad-h /b /s 2>nul)))
LIBF =

# Compilation configs
CC = gcc
CXX = g++
CFLAGS +=
CXXFLAGS +=
CPPFLAGS += $(WARNINGS) $(DMACROS) $(INCDIRS)
LDFLAGS += $(LIBDIRS) $(LIBF)

.PHONY: all debug clean distclean

all: release.exe
debug: debug.exe

release.exe: $(OBJS)
ifneq ($(CPPSRC),)
	$(strip $(CXX) $^ -o $@ $(LDFLAGS))
endif
ifeq ($(CPPSRC),)
	$(strip $(CC) $^ -o $@ $(LDFLAGS))
endif

debug.exe: $(subst src\,debug\,$(OBJS))
ifneq ($(CPPSRC),)
	$(strip $(CXX) $^ -o $@ $(LDFLAGS))
endif
ifeq ($(CPPSRC),)
	$(strip $(CC) $^ -o $@ $(LDFLAGS))
endif

obj\src\\%.c.o: src\\%.c
	$(shell if not exist "$(dir $@)" mkdir $(dir $@))
	$(strip $(CC) $(CFLAGS) $(CPPFLAGS) $(OPTIMS) -c $< -o $@)

obj\src\\%.cpp.o: src\\%.cpp
	$(shell if not exist "$(dir $@)" mkdir $(dir $@))
	$(strip $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(OPTIMS) -c $< -o $@)

obj\debug\\%.c.o: src\\%.c
	$(shell if not exist "$(dir $@)" mkdir $(dir $@))
	$(strip $(CC) $(CFLAGS) $(CPPFLAGS) $(DBGFLAGS) -c $< -o $@)

obj\debug\\%.cpp.o: src\\%.cpp
	$(shell if not exist "$(dir $@)" mkdir $(dir $@))
	$(strip $(CXX) $(CXXFLAGS) $(CPPFLAGS) $(DBGFLAGS) -c $< -o $@)

clean:
	$(shell if exist "obj" rmdir /s /q obj 2> nul)
	del *.exe 2> nul
