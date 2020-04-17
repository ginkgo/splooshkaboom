
all: splooshkaboom

splooshkaboom: splooshkaboom.cpp
	g++ --std=c++2a -DNDEBUG -mbmi2 -O2 splooshkaboom.cpp -o splooshkaboom
