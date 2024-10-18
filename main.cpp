#include <iostream>
#include <cassert>
#include <chrono>
#include <vector>
#include <random>
#include <cstdint>
#include <limits>
#include <iomanip>
#include <unordered_map>
#include <cstring>

#ifndef _OPENMP
#error "OpenMP not defined, retry by adding '-fopenmp' to compile options."
#endif

#include <omp.h>

using element_type = uint32_t;

template <typename T>
void swap(T* a, T* b) {
	const T tmp = *a;
	*a = *b;
	*b = tmp;
}

template <typename T>
void merge(T* v, T* tmp, const size_t left_length, const size_t right_length) {
	// This function assumes that 'v' is the pointer to the element of two
	// contiguous (non-overlapping) blocks of memory of length 'left_length' and
	// 'right_length' respectively. 'tmp' is a pointer to a memory region with
	// the same size to be used as "temporary working space" to sort the array.

	T* right = v + left_length;
	size_t i{0};
	size_t j{0};
	size_t k{0};

	while (i < left_length && j < right_length) {
		if (v[i] < right[j]) {
			tmp[k] = v[i];
			i++;
		} else {
			tmp[k] = right[j];
			j++;
		}
		k++;
	}

	while (i < left_length) {
		tmp[k] = v[i];
		i++;
		k++;
	}

	while (j < right_length) {
		tmp[k] = right[j];
		j++;
		k++;
	}

	// copy back the temporary array
	std::memcpy(v, tmp, (left_length + right_length) * sizeof(T));
}

template <typename T>
void merge_sort(T* v, T* tmp, const size_t n) {
	if (n <= 1) {
		return;
	}

	const size_t left_length = n / 2;
	const size_t right_length = n - left_length;
	assert(left_length + right_length == n);

	merge_sort(v, tmp, left_length);
	merge_sort(v + left_length, tmp + left_length, right_length);
	merge(v, tmp, left_length, right_length);
}

template <typename T>
void check(const std::vector<T>& original, const std::vector<T>& v) {
	// This function does not only check that v is sorted but it checks that v
	// has the same elements of 'original'

	if (original.size() != v.size()) {
		throw std::runtime_error("Vectors have different sizes: expected " +
								 std::to_string(original.size()) + " but was " +
								 std::to_string(v.size()));
	}

	for (size_t i{0}; i < v.size() - 1; i++) {
		if (v.at(i + 1) < v.at(i)) {
			throw std::runtime_error("Vector is not sorted: element at index " +
									 std::to_string(i) + " (" +
									 std::to_string(v[i]) +
									 ") is greater than next element (" +
									 std::to_string(v[i + 1]) + ")");
		}
	}

	std::unordered_map<T, uint64_t> count_original;
	for (const T x : original) {
		if (count_original.find(x) == count_original.end()) {
			count_original[x] = 1;
		} else {
			count_original[x]++;
		}
	}

	std::unordered_map<T, uint64_t> count_v;
	for (const T x : v) {
		if (count_v.find(x) == count_v.end()) {
			count_v[x] = 1;
		} else {
			count_v[x]++;
		}
	}

	if (count_original.size() != count_v.size()) {
		throw std::runtime_error(
			"Different number of unique elements in vectors: expected " +
			std::to_string(count_original.size()) + " but were " +
			std::to_string(count_v.size()));
	}

	for (auto const& [key, val] : count_original) {
		if (count_v.find(key) == count_v.end()) {
			throw std::runtime_error(
				"Original vector contained a key (" + std::to_string(key) +
				") which did not appear in the other vector.");
		}
		if (count_v[key] != val) {
			throw std::runtime_error("Wrong number of elements (" +
									 std::to_string(key) + "): expected " +
									 std::to_string(val) + " but were " +
									 std::to_string(count_v[key]));
		}
	}
}

int main(int argc, char* argv[]) {
	std::random_device dev;
	std::mt19937 rnd{dev()};
	std::uniform_int_distribution<element_type> dist{
		std::numeric_limits<element_type>::min(),
		std::numeric_limits<element_type>::max()};

	std::ios default_io_state(nullptr);
	default_io_state.copyfmt(std::cout);

	std::cout << "Type of one element: " << typeid(element_type).name()
			  << std::endl;
	std::cout << "Size of one element: " << sizeof(element_type) << " bytes"
			  << std::endl;
	std::cout << "Min value: " << std::numeric_limits<element_type>::min()
			  << " (0x" << std::hex << std::setw(2 * sizeof(element_type))
			  << std::setfill('0') << std::numeric_limits<element_type>::min()
			  << ")" << std::endl;
	std::cout.copyfmt(default_io_state);
	std::cout << "Max value: " << std::numeric_limits<element_type>::max()
			  << " (0x" << std::hex << std::setw(2 * sizeof(element_type))
			  << std::setfill('0') << std::numeric_limits<element_type>::max()
			  << ")" << std::endl;
	std::cout.copyfmt(default_io_state);

	size_t n_elements = 100;
	if (argc > 1) {
		n_elements = strtoul(argv[1], NULL, 10);
		if (n_elements < 1) {
			std::cerr << "Error: Number of elements must be a positive integer."
					  << std::endl;
			std::exit(-1);
		}
	}

	if (argc > 2) {
		std::cerr << "WARNING: passed more arguments than needed." << std::endl;
	}

	std::cout << "Using " << n_elements << " elements ("
			  << (n_elements * sizeof(element_type)) << " bytes)" << std::endl;

	std::vector<element_type> original(n_elements);
	for (size_t i{0}; i < n_elements; i++) {
		original[i] = dist(rnd);
	}

	// copy
	std::vector<element_type> v = original;
	std::vector<element_type> tmp(n_elements);

	const double start = omp_get_wtime();
	merge_sort(v.data(), tmp.data(), v.size());
	const double end = omp_get_wtime();
	const double elapsed_seconds = end - start;
	std::cout << "Time: " << elapsed_seconds << " seconds" << std::endl;

	check(original, v);

	return 0;
}