#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <random>
#include <cstdint>
#include <limits>

#ifndef _OPENMP
#error "OpenMP not defined, retry by adding '-fopenmp' to compile options."
#endif

#include <omp.h>

using element_type = uint32_t;

int main(void) {
	std::random_device dev;
	std::mt19937 rnd{dev()};
	std::uniform_int_distribution<element_type> dist{
		static_cast<element_type>(0), std::numeric_limits<element_type>::max()};

	for (int i = 0; i < 10; i++) {
		std::cout << dist(rnd) << std::endl;
	}

	return 0;
}