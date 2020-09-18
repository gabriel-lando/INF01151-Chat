CXX       := g++
CXX_FLAGS := -std=c++17 -ggdb

SRC_DIR     := src
INCLUDE := include

LIBRARIES   :=

## Executables
CLIENT_EXE := bin/client
SERVER_EXE := bin/server

## Sources Client
RAW_CLIENTS_SRCS := client.cpp
CLIENT_SRCS = $(addprefix $(SRC_DIR)/, $(RAW_CLIENTSRCS))

## Sources Server
RAW_SERVER_SRCS := server.cpp
SERVER_SRCS = $(addprefix $(SRC_DIR)/, $(RAW_SERVER_SRCS))

## Sources used by both
RAW_SRCS = 
SRCS = $(addprefix $(SRC_DIR)/, $(RAW_SRCS))

## Object files
RAW_CLIENT_OBJS := $(RAW_CLIENT_SRCS:%.cpp=%.o)
CLIENT_OBJS := $(addprefix $(SRC)/, $(RAW_CLIENT_SRCS))
RAW_SERVER_OBJS := $(RAW_SERVER_SRCS:%.cpp=%.o)
SERVER_OBJS := $(addprefix $(SRC)/, $(RAW_SERVER_SRCS))
RAW_OBJS := $(RAW_SRCS:%.cpp=%.o)
OBJS := $(addprefix $(BUILDDIR)/, $(RAW_OBJS))

all: $(CLIENT_EXE) $(SERVER_EXE)

$(CLIENT_EXE): $(OBJS) $(CLIENT_OBJS)
	@echo " ";
	@echo " Link client:";
	$(CC) $^ -o $(CLIENT_EXE) $(LIB)

$(SERVER_EXE): $(OBJS) $(SERVER_OBJS)
	@echo " ";
	@echo " Link server:";
	$(CC) $^ -o $(SERVER_EXE) $(LIB)

$(BUILDDIR)/%.o: $(SRC)/%.cpp
	@echo " ";
	@echo " Compile $<";
	$(CC) $(CFLAGS) $(INC) -c -o $@ $<

run: clean all
	./$(BIN)/$(EXECUTABLE)

$(BIN)/$(EXECUTABLE): $(SRC)/client.cpp
	$(CXX) $(CXX_FLAGS) -I$(INCLUDE) $^ -o $@ $(LIBRARIES)

clean:
	-rm $(BIN)/*
