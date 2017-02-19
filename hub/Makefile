CFLAGS = -std=c++11 -Wextra -pedantic -MD -MP
SRC = radio.cpp nrf905.cpp socket.cpp poller.cpp
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
