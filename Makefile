# Compiler options 
CC := g++ -std=c++17
CFLAGS = -g -Wall -Wextra -Werror
LIB := -pthread
INC := -I include

# Macros
MKDIR := mkdir -p
RM := rm -rf

# Project dirs
SRCDIR := src
BINDIR := bin
BUILDDIR := build

# Project files
## Executables
CLIENTEXE := bin/client
SERVEREXE := bin/server
FRONTEXE  := bin/front
## Sources
RAWCLIENTSRCS := client.cpp
CLIENTSRCS = $(addprefix $(SRCDIR)/, $(RAWCLIENTSRCS))
RAWSERVERSRCS := server.cpp
SERVERSRCS = $(addprefix $(SRCDIR)/, $(RAWSERVERSRCS))
RAWFRONTSRCS := front.cpp
FRONTSRCS = $(addprefix $(SRCDIR)/, $(RAWFRONTSRCS))
## Sources used by both
RAWSRCS = helper.cpp io.cpp client_socket.cpp server_socket.cpp
SRCS = $(addprefix $(SRCDIR)/, $(RAWSRCS))
## Object files
RAWCLIENTOBJS := $(RAWCLIENTSRCS:%.cpp=%.o)
CLIENTOBJS := $(addprefix $(SRCDIR)/, $(RAWCLIENTSRCS))
RAWSERVEROBJS := $(RAWSERVERSRCS:%.cpp=%.o)
SERVEROBJS := $(addprefix $(SRCDIR)/, $(RAWSERVERSRCS))
RAWFRONTOBJS := $(RAWFRONTSRCS:%.cpp=%.o)
FRONTOBJS := $(addprefix $(SRCDIR)/, $(RAWFRONTSRCS))
RAWOBJS := $(RAWSRCS:%.cpp=%.o)
OBJS := $(addprefix $(BUILDDIR)/, $(RAWOBJS))

.PHONY: directories

# Rules
all: directories $(CLIENTEXE) $(SERVEREXE) $(FRONTEXE)
	@echo " ";
	@echo " Done!"

debug: CFLAGS += -DDEBUG
debug: all
	@echo " Debug mode"

$(CLIENTEXE): $(OBJS) $(CLIENTOBJS)
	@echo " ";
	@echo " Link client:";
	$(CC) $^ -o $(CLIENTEXE) $(LIB)

$(SERVEREXE): $(OBJS) $(SERVEROBJS)
	@echo " ";
	@echo " Link server:";
	$(CC) $^ -o $(SERVEREXE) $(LIB)

$(FRONTEXE): $(OBJS) $(FRONTOBJS)
	@echo " ";
	@echo " Link front:";
	$(CC) $^ -o $(FRONTEXE) $(LIB)

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@echo " ";
	@echo " Compile $<";
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

directories:
	$(MKDIR) $(BUILDDIR)
	$(MKDIR) $(BINDIR)

# Tests
#tester:
#	$(CC) $(CFLAGS) test/tester.cpp $(INC) $(LIB) -o bin/tester.exe

clean:
	@echo " Cleaning...";
	$(RM) $(BUILDDIR)/*.o $(CLIENTEXE) $(SERVEREXE) $(BUILDDIR) $(BINDIR);
	@echo " Cleaned!"

rebuild: clean all
redebug: clean debug