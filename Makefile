CXX=g++
CXX_FLAGS=--std=c++17 -Wall -Wextra -Wpedantic -Wshadow -Wsign-conversion -Wfloat-conversion -Werror
DEBUG_FLAGS=-O0 -g -fsanitize=address,undefined,leak
OPT_FLAGS=-O3 -DNDEBUG -march=native -mtune=native

debug: main.cpp
	$(CXX) $(CXX_FLAGS) $(DEBUG_FLAGS) main.cpp -o main.x -fopenmp

release: main.cpp
	$(CXX) $(CXX_FLAGS) $(OPT_FLAGS) main.cpp -o main.x -fopenmp

clean:
	rm main.x
