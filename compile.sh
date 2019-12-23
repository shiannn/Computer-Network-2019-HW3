#! /bin/bash

g++ myreceiver.cpp -o myreceiver `pkg-config --cflags --libs opencv`
g++ mysender.cpp -o mysender `pkg-config --cflags --libs opencv`
g++ agent.cpp -o agent