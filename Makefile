# Makefile
# -----------------------------------------------------------------------------

NAME	:= webserv

CXX 		:= c++
CXXFLAGS 	:= -Wall -Wextra -Werror -std=c++98
CPPFLAGS	:= -I inc -MMD -MP

LOG_LEVEL	?= INFO
VALID_LOG_LEVELS := DEBUG INFO WARNING ERROR CRITICAL
ifeq ($(filter $(LOG_LEVEL),$(VALID_LOG_LEVELS)),)
$(error LOG_LEVEL inválido: '$(LOG_LEVEL)'. Usa: $(VALID_LOG_LEVELS))
endif
CPPFLAGS	+= -DLOG_LEVEL=$(LOG_LEVEL)

SRC_DIR		:= src
OBJ_DIR		:= obj

SRCS	:= 	main.cpp \
			Logger.cpp log_global.cpp \
			ConfigParser.cpp LocationConfig.cpp ServerConfig.cpp \
			ListeningSocket.cpp Buffer.cpp Connection.cpp HttpParser.cpp \
			Request.cpp RequestLine.cpp OriginForm.cpp Headers.cpp AbsoluteForm.cpp \
			Response.cpp Decision.cpp http_status_codes.cpp \
			path_utils.cpp Processor.cpp

OBJS	:= $(SRCS:%.cpp=$(OBJ_DIR)/%.o)
DEPS	:= $(OBJS:.o=.d)

TEST_DIR 	:= tests
TEST_NAME 	:= run_tests


TEST_DIR	:= tests
TEST_SRCS	:= Tester.cpp test_main.cpp \
			   logger_test.cpp  \
			   config_parser_test.cpp \
			   buffer_test.cpp \
			   test_httpParser.cpp \
			   test_bodyParser.cpp \
			   test_chunkParser.cpp \
			   test_requestParser.cpp \
			   processor_test.cpp \
			   ConfigDirectiveTests.cpp\
			   test_errorPage.cpp

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
