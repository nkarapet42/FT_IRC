NAME = ircserv

CPP = c++

INCLUDES =  Server.hpp \
			Client.hpp \
			Info.hpp \
			Channel.hpp

CFLAGS = -Wall -Wextra -Werror -std=c++98

SRCS =  main.cpp \
		Server.cpp \
		Client.cpp

OBJS = $(SRCS:.cpp=.o)

all: $(NAME)

$(NAME): $(OBJS)
	$(CPP) $(CFLAGS) -o $(NAME) $(OBJS)

%.o: %.cpp Makefile $(INCLUDES)
	$(CPP) $(CFLAGS) -c $< -o $@

clean:
	rm -f $(OBJS)

fclean: clean
	rm -f $(NAME)

re: fclean all