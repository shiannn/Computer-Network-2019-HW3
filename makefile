CC = g++
OPENCV =  `pkg-config --cflags --libs opencv`

RECEIVER = receiver.cpp
SENDER = sender.cpp
AGENT = agent.cpp
RECEI = receiver
SEND = sender
AGE = agent

all: sender receiver agent
  
sender: $(SENDER)
	$(CC) $(SENDER) -o $(SEND)  $(OPENCV)
receiver: $(RECEIVER)
	$(CC) $(RECEIVER) -o $(RECEI)  $(OPENCV)
agent: $(AGENT)
	$(CC) $(AGENT) -o $(AGE) $(OPENCV)

.PHONY: clean

clean:
	rm $(RECEI) $(SEND) $(AGE)