SRCS=$(shell find ./* -type f -name '*.cpp')
OBJS=$(SRCS:.cpp=.o) 
CFLAGS= -Wall
LDFLAGS= -lstdc++ -lpthread -lwiringPi -lssl -lcrypto

TARGET=doorlock

all: $(TARGET)
	$(CXX) -o $(TARGET) $(OBJS) $(LDFLAGS)

$(TARGET):
	$(CXX) -c $(CFLAGS) $(SRCS)

.PHONY:
	clean

clean:
	$(RM) *.o $(TARGET)
