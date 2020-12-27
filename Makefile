QUIET=@
MKDIR=$(QUIET) mkdir -p
ECHO=$(QUIET)echo
PKG_CONFIG=pkg-config
SRC = \
      src/osmium_import.cpp \
      src/output.cpp \
      src/output_geojson.cpp \
      src/output_paths.cpp \
      src/import_paths.cpp \
      src/helper.cpp \
      src/checks.cpp \
      src/graph.cpp

SRC_LIBS = \
     3rd_party/clipper/clipper.cpp

SRC_CLI = \
     src/main.cpp

SRC_PYTHON = \
     src/python_module.cpp

INC = \
      -I3rd_party/clipper

BASE_BUILD_DIR=build
OBJDIR=$(BUILD_DIR)

OBJ = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))
OBJ_CLI = $(addprefix $(OBJDIR)/,$(SRC_CLI:.cpp=.o))
OBJ_PYTHON = $(addprefix $(OBJDIR)/,$(SRC_PYTHON:.cpp=.o))
OBJ_LIBS = $(addprefix $(OBJDIR)/,$(SRC_LIBS:.cpp=.o))

CXX_FLAGS_BASE= --std=c++17 $(INC) -MP -MMD
CXX_FLAGS= $(CXX_FLAGS_BASE) -lpthread -lz -lexpat -lbz2
LIBS_BASE=
LIBS_PYTHON= $(LIBS_BASE) python3

DEBUG=0
ifeq ($(DEBUG), 0)
	CXX_FLAGS += -O3
	BUILD_DIR = $(BASE_BUILD_DIR)/release
else
	CXX_FLAGS += -O0 -g3
	BUILD_DIR = $(BASE_BUILD_DIR)/debug
endif

all: $(BASE_BUILD_DIR)/a.out $(BASE_BUILD_DIR)/backend.so $(BASE_BUILD_DIR)/__init__.py

$(BASE_BUILD_DIR)/a.out: $(BUILD_DIR)/a.out
	cp $< $@

$(BASE_BUILD_DIR)/backend.so: $(BUILD_DIR)/backend.so
	cp $< $@

$(BASE_BUILD_DIR)/__init__.py:
	touch $@

$(BUILD_DIR)/a.out: $(OBJ) $(OBJ_LIBS) $(OBJ_CLI)
	$(ECHO) $@
	$(MKDIR) $(dir $@)
	$(QUIET)g++ $(CXX_FLAGS) $^ -o $@

$(BUILD_DIR)/backend.so: $(OBJ) $(OBJ_LIBS) $(OBJ_PYTHON)
	$(ECHO) $@
	$(MKDIR) $(dir $@)
	$(QUIET)g++ $(CXX_FLAGS) $^ -shared $(shell $(PKG_CONFIG) --libs $(LIBS_PYTHON)) -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(ECHO) $@
	$(MKDIR) $(dir $@)
	$(QUIET)g++ -c $(if $(strip $(LIBS)),$(shell $(PKG_CONFIG) --cflags $(LIBS)),) $(CXX_FLAGS) $< -o $@

$(OBJ_PYTHON): LIBS = $(LIBS_PYTHON)
$(OBJ_LIBS): LIBS=
$(OBJ_LIBS): CXX_FLAGS=$(CXX_FLAGS_BASE)


clean:
	rm -rf $(BUILD_DIR)

cleanAll:
	rm -rf $(BASE_BUILD_DIR)


-include  $(OBJ:.o=.d)

.PHONY:clean
.PHONY:cleanAll
