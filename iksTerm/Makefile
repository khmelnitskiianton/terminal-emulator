#compiler
#>>> PASS CC=<your compiler>
ifeq ($(origin CC),default)
	CC=gcc
endif
#Name of compiled executable
NAME = iksTerm
#name of dir where all build files will be stored
BUILDDIR = build
#Name of directory where .o and .d files will be stored
OBJDIR = $(BUILDDIR)/obj
#Name of directory with headers
INCLUDEDIR = include
#Name of directory with .c
SRCDIR = src
#Name of json file to store info about generaions for clangd
COMPILEJSON = compile_commands.json
#Install dir
PREFIX ?= $(HOME)/.local
INSTALL_DIR := $(PREFIX)/bin
# Use MAN_DIR from environment if set, otherwise default to /usr/local/share/man/man1
MAN_DIR ?= /usr/local/share/man/man1


#CFLAGS_DEBUG_LINUX = -D _DEBUG -DDEBUG -DNDEBUG -ggdb3 -O0 -Wall -Wextra										\
#				-Waggressive-loop-optimizations -Wmissing-declarations -Wcast-align 							\
#				-Wcast-qual -Wchar-subscripts -Wconversion  													\
#				-Wempty-body -Wfloat-equal -Wformat-nonliteral -Wformat-security -Wformat-signedness 			\
#				-Wformat=2 -Winline -Wlogical-op -Wopenmp-simd  												\
#				-Wpacked -Wpointer-arith -Winit-self -Wredundant-decls -Wshadow -Wsign-conversion 				\
#				-Wstrict-overflow=2 																			\
#				-Wsuggest-final-methods -Wsuggest-final-types -Wswitch-default 									\
#				-Wswitch-enum -Wsync-nand -Wundef -Wunreachable-code -Wunused 									\
#				-Wvariadic-macros  -Wno-missing-field-initializers 												\
#				-Wno-narrowing -Wno-varargs -Wstack-protector -fcheck-new 										\
#				-fstack-protector -fstrict-overflow 															\
#				-fno-omit-frame-pointer -Wlarger-than=80000 -Wstack-usage=8192 -pie -fPIE -Werror=vla 			\
#				-fsanitize=address,alignment,bool,bounds,enum,float-cast-overflow,float-divide-by-zero,$\
#				integer-divide-by-zero,leak,nonnull-attribute,null,object-size,return,$\
#				returns-nonnull-attribute,shift,signed-integer-overflow,undefined,unreachable,vla-bound,vptr
CFLAGS_RELEASE_LINUX = -O3

CMD_DEL = @rm -rf ./$(OBJDIR)/*.o ./$(OBJDIR)/*.d ./$(BUILDDIR)/*.json
CMD_MKDIR = @mkdir -p $(BUILDDIR) $(OBJDIR)

override CFLAGS += -I./$(INCLUDEDIR) $(CFLAGS_DEBUG_LINUX) $(CFLAGS_RELEASE_LINUX)
override LDFLAGS += -lX11

# Debug build settings
DBGDIR = debug
DBGEXE = $(DBGDIR)/$(NAME)
# Release build settings
RELDIR = bin
RELEXE = $(RELDIR)/$(NAME)

.PHONY: all init compile clean debug release install man

all: init release

release: $(RELEXE)
debug: $(DBGEXE)

#Note: ALL .c in source dir will be compiled
#Getting all .c
SRCS := $(wildcard $(SRCDIR)/*.c)
#Replacing .c with .o, temporary variable
TOBJS := $(SRCS:%.c=%.o)
#Replacing src dir to obj dir
OBJS := $(TOBJS:$(SRCDIR)%=$(OBJDIR)%)
#Dependencies for .c files, they are stored with .o objects
DEPS := $(OBJS:%.o=%.d)

#====================================================================
init:
	@mkdir -p $(BUILDDIR) $(OBJDIR) $(RELDIR)

compile: init
	bear --output $(BUILDDIR)/$(COMPILEJSON) -- make release

#Deletes all object and .d files
clean:
	$(CMD_DEL)

install: man
	@echo "Installing $(RELEXE) to $(INSTALL_DIR)"
	@install -d $(INSTALL_DIR)
	@install -m 755 $(RELEXE) $(INSTALL_DIR)
	@echo "Installation complete"

man:
	sudo cp ./iksTerm.1 $(MAN_DIR)/
	sudo mandb

#====================================================================

#Main target to compile executable
$(DBGEXE): $(OBJS)
	@mkdir -p $(BUILDDIR) $(OBJDIR)
	@mkdir -p $(DBGDIR)
	$(CC) $(CFLAGS) $^ -o $@
$(RELEXE): $(OBJS)
	@mkdir -p $(BUILDDIR) $(OBJDIR)
	@mkdir -p $(RELDIR)
	$(CC) $(CFLAGS) $^ -o $@ $(LDFLAGS)

#Automatic target to compile object files
$(OBJS) : $(OBJDIR)/%.o : $(SRCDIR)/%.c
	$(CMD_MKDIR)
	$(CC) $(CFLAGS) -c $< -o $@

#Idk how it works, but is uses compiler preprocessor to automatically generate
#.d files with included headers that make can use
$(DEPS) : $(OBJDIR)/%.d : $(SRCDIR)/%.c
	$(CMD_MKDIR)
	$(CC) -E $(CFLAGS) $< -MM -MT $(@:.d=.o) > $@

#====================================================================
# Var consist of all aims that dont need to update deps!!!
NODEPS = clean init install man

#Includes make dependencies
ifeq (0, $(words $(findstring $(MAKECMDGOALS), $(NODEPS))))
include $(DEPS)
endif