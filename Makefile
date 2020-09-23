# Compiler options 
CC := g++ -std=c++17
CFLAGS = -g -Wall -Wextra -Werror
LIB := -pthread
INC := -I include

# Project dirs
SRCDIR := src
BUILDDIR := build

# Project files
## Executables
CLIENTEXE := bin/client
SERVEREXE := bin/server
## Sources
RAWCLIENTSRCS := client.cpp
CLIENTSRCS = $(addprefix $(SRCDIR)/, $(RAWCLIENTSRCS))
RAWSERVERSRCS := server.cpp
SERVERSRCS = $(addprefix $(SRCDIR)/, $(RAWSERVERSRCS))
## Sources used by both
RAWSRCS = helper.cpp
SRCS = $(addprefix $(SRCDIR)/, $(RAWSRCS))
## Object files
RAWCLIENTOBJS := $(RAWCLIENTSRCS:%.cpp=%.o)
CLIENTOBJS := $(addprefix $(SRCDIR)/, $(RAWCLIENTSRCS))
RAWSERVEROBJS := $(RAWSERVERSRCS:%.cpp=%.o)
SERVEROBJS := $(addprefix $(SRCDIR)/, $(RAWSERVERSRCS))
RAWOBJS := $(RAWSRCS:%.cpp=%.o)
OBJS := $(addprefix $(BUILDDIR)/, $(RAWOBJS))

# Rules
all: $(CLIENTEXE) $(SERVEREXE)
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

$(BUILDDIR)/%.o: $(SRCDIR)/%.cpp
	@echo " ";
	@echo " Compile $<";
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

# Tests
#tester:
#	$(CC) $(CFLAGS) test/tester.cpp $(INC) $(LIB) -o bin/tester.exe

clean:
	@echo " Cleaning...";
	$(RM) $(BUILDDIR)/*.o $(CLIENTEXE) $(SERVEREXE);
	@echo " Cleaned!"

rebuild: clean all
redebug: clean debug