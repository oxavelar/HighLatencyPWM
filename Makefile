PROGRAM := pwm

CC = g++
CXXFLAGS += -O2 -Wall -Wextra -Werror -pedantic-errors -std=c++11 -pipe
LDLIBS += -lpthread -lboost_system -lboost_filesystem

SOURCES = demo.cpp PWM.cc
OBJECTS = demo.o PWM.o

all: $(SOURCES) $(OBJECTS)
	$(CC) $(CXXFLAGS) $(LDLIBS) $(OBJECTS) -o $(PROGRAM)

clean:
	rm -rf $(PROGRAM) $(OBJECTS)

