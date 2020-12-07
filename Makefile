QUIET=@
MKDIR=$(QUIET) mkdir -p
SRC = \
     backend.cpp \
     3rd_party/clipper/clipper.cpp

INC = \
      -I3rd_party/clipper

BUILD_DIR=build
OBJDIR=$(BUILD_DIR)

OBJ = $(addprefix $(OBJDIR)/,$(SRC:.cpp=.o))

CXX_FLAGS= -O0 -g3 -lpthread -lz -lexpat -lbz2 --std=c++17 $(INC)

a.out: $(OBJ)
	$(MKDIR) $(dir $@)
	g++ $(CXX_FLAGS) $^ -o $@

$(BUILD_DIR)/3rd_party/%.o: 3rd_party/%.cpp
	$(MKDIR) $(dir $@)
	g++ -c --std=c++17 $< -o $@

$(BUILD_DIR)/%.o: %.cpp
	$(MKDIR) $(dir $@)
	g++ -c $(CXX_FLAGS) $< -o $@


clean:
	rm -rf $(BUILD_DIR)

.PHONY:clean
