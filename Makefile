NAME = ircserv
CXX = c++
CXXFLAGS =  -std=c++98 # -Wall -Wextra -Werror
RM = rm -f

SRC = Server.cpp \
	  Client.cpp \
	  ClientManager.cpp \
	  ChannelManager.cpp \
	  Channel.cpp \
	  parser.cpp \
	  main.cpp \
	  ParsedCommand.cpp

OBJ = $(SRC:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJ)
	$(CXX) $(CXXFLAGS) $(OBJ) -o $(NAME)

%.o: %.cpp
	$(CXX) $(CXXFLAGS) -c $< -o $@

clean:
	$(RM) $(OBJ)

fclean: clean
	$(RM) $(NAME)

re: fclean all

.PHONY: all clean fclean re
