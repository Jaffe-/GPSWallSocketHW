CFLAGS = -std=c++11 -Wextra -pedantic -MD -MP
SRC = ../node/protocol.cpp hub.cpp nrf905.cpp socket.cpp
OBJ = $(SRC:.cpp=.o)
TARGET = radio

all: $(TARGET)

$(TARGET) : $(OBJ)
	g++ $^ -o $@

%.o : %.cpp
	g++ $(CFLAGS) -c $< -o $@

-include $(SRC:%.cpp=%.d)

clean:
	rm -f $(OBJ) $(SRC:%.cpp=%.d)

.PHONY: all clean
