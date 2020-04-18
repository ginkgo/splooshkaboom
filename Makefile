
all: splooshkaboom splooshkaboom_debug

splooshkaboom: splooshkaboom.cpp
	g++ --std=c++17 -DNDEBUG -mbmi2 -O2 splooshkaboom.cpp -o splooshkaboom

splooshkaboom_debug: splooshkaboom.cpp
	g++ --std=c++17 -mbmi2 -g -O0 splooshkaboom.cpp -o splooshkaboom_debug
