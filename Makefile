
all: splooshkaboom

splooshkaboom: splooshkaboom.cpp
	g++ --std=c++17 -DNDEBUG -mbmi2 -O2 splooshkaboom.cpp -o splooshkaboom
