# Makefile
# -----------------------------------------------------------------------------

NAME	:= webserv

CXX 		:= c++
CXXFLAGS 	:= -Wall -Wextra -Werror -std=c++98
CPPFLAGS	:= -I inc -MMD -MP

SRC_DIR		:= src
OBJ_DIR		:= obj

SRCS	:= 	main.cpp \
			Logger.cpp \
			ConfigParser.cpp LocationConfig.cpp ServerConfig.cpp
OBJS	:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS	:= $(OBJS:.o=.d)

TEST_DIR 	:= tests
TEST_NAME 	:= run_tests


TEST_DIR	:= tests
TEST_SRCS	:= Tester.cpp test_main.cpp \
			   logger_test.cpp  \
			   config_parser_test.cpp
TEST_OBJS	:= $(TEST_SRCS:%.cpp=$(OBJ_DIR)/tests/%.o)
DEPS		+= $(TEST_OBJS:.o=.d)

LIB_OBJS	:= $(filter-out $(OBJ_DIR)/main.o, $(OBJS))

all: $(NAME)

$(NAME): $(OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@

$(OBJ_DIR)/%.o: $(SRC_DIR)/%.cpp | $(OBJ_DIR)
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR):
	mkdir -p $@

clean:
	rm -rf $(OBJ_DIR)

fclean: clean
	rm -f $(NAME) $(TEST_NAME)

re:
	$(MAKE) fclean
	$(MAKE) all

test: $(TEST_NAME)
	./$(TEST_NAME) $(ARGS)

$(TEST_NAME): $(TEST_OBJS) $(LIB_OBJS)
	$(CXX) $(CXXFLAGS) $^ -o $@
	
$(OBJ_DIR)/tests/%.o: $(TEST_DIR)/%.cpp | $(OBJ_DIR)/tests
	$(CXX) $(CXXFLAGS) $(CPPFLAGS) -c $< -o $@

$(OBJ_DIR)/tests:
	mkdir -p $@

.PHONY: all clean fclean re test
