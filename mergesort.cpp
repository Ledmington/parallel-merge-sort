/*
 *  This file is part of Christian's OpenMP software lab
 *
 *  Copyright (C) 2016 by Christian Terboven <terboven@itc.rwth-aachen.de>
 *  Copyright (C) 2016 by Jonas Hahnfeld <hahnfeld@itc.rwth-aachen.de>
 *
 *  This program is free software; you can redistribute it and/or modify
 *  it under the terms of the GNU General Public License as published by
 *  the Free Software Foundation; either version 2 of the License, or
 *  (at your option) any later version.
 *
 *  This program is distributed in the hope that it will be useful,
 *  but WITHOUT ANY WARRANTY; without even the implied warranty of
 *  MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
 *  GNU General Public License for more details.
 *
 *  You should have received a copy of the GNU General Public License
 *  along with this program; if not, write to the Free Software
 *  Foundation, Inc., 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA
 *
 */

#include <stdio.h>
#include <stdlib.h>
#include <errno.h>
#include <sys/time.h>

#include <iostream>
#include <algorithm>

#include <cstdlib>
#include <cstdio>

#include <cmath>
#include <ctime>
#include <cstring>

#include <omp.h>

/**
 * helper routine: check if array is sorted correctly
 */
bool isSorted(int ref[], int data[], const size_t size) {
	bool sorted = true;
	std::sort(ref, ref + size);
	for (size_t idx = 0; idx < size; ++idx) {
		if (ref[idx] != data[idx]) {
			sorted = false;
		}
	}
	return sorted;
}

/**
 * sequential merge step (straight-forward implementation)
 */
void MsMergeSequential(int *out, int *in, long begin1, long end1, long begin2,
					   long end2, long outBegin) {
	long left = begin1;
	long right = begin2;

	long idx = outBegin;

	while (left < end1 && right < end2) {
		if (in[left] <= in[right]) {
			out[idx] = in[left];
			left++;
		} else {
			out[idx] = in[right];
			right++;
		}
		idx++;
	}

	while (left < end1) {
		out[idx] = in[left];
		left++, idx++;
	}

	while (right < end2) {
		out[idx] = in[right];
		right++, idx++;
	}
}

/**
 * sequential MergeSort
 */
void MsSequential(int *array, int *tmp, bool inplace, long begin, long end,
				  long depth) {
	if (begin < (end - 1)) {
		const long half = (begin + end) / 2;

		if (depth == 0) {
			// when depth reaches 0, we start doing things serially and
			// recursively
			MsSequential(array, tmp, !inplace, begin, half, 0);
			MsSequential(array, tmp, !inplace, half, end, 0);
		} else {
#pragma omp task
			{ MsSequential(array, tmp, !inplace, begin, half, depth - 1); }
#pragma omp task
			{ MsSequential(array, tmp, !inplace, half, end, depth - 1); }
#pragma omp taskwait
		}

		if (inplace) {
			MsMergeSequential(array, tmp, begin, half, half, end, begin);
		} else {
			MsMergeSequential(tmp, array, begin, half, half, end, begin);
		}
	} else if (!inplace) {
		tmp[begin] = array[begin];
	}
}

/**
 * Parallel MergeSort
 */
void MsParallel(int *array, int *tmp, const size_t size) {
	// Computing depth as log2(n_threads)
	size_t depth = 0;
	{
		size_t x = omp_get_max_threads();
		while (x > 0) {
			x >>= 1;
			depth++;
		}
		depth--;
	}

#pragma omp parallel default(none) shared(array, tmp, size) firstprivate(depth)
#pragma omp single nowait
	{ MsSequential(array, tmp, true, 0, size, depth); }
}

/**
 * @brief program entry point
 */
int main(int argc, char *argv[]) {
	// variables to measure the elapsed time
	struct timeval t1, t2;
	double etime;

	// expect one command line arguments: array size
	if (argc != 2) {
		printf("Usage: %s <array size>\n", argv[0]);
		printf("\n");
		return EXIT_FAILURE;
	} else {
		const size_t stSize = strtol(argv[1], NULL, 10);
		int *data = (int *)malloc(stSize * sizeof(int));
		int *tmp = (int *)malloc(stSize * sizeof(int));
		int *ref = (int *)malloc(stSize * sizeof(int));

		printf("Initialization...\n");

		srand(95);
		for (size_t idx = 0; idx < stSize; ++idx) {
			data[idx] = (int)(stSize * (double(rand()) / RAND_MAX));
		}
		std::copy(data, data + stSize, ref);

		double dSize =
			static_cast<double>(stSize * sizeof(int)) / 1024.0 / 1024.0;
		printf("Sorting %zu elements of type int (%f MiB)...\n", stSize, dSize);

		gettimeofday(&t1, NULL);
		MsParallel(data, tmp, stSize);
		gettimeofday(&t2, NULL);

		etime =
			(t2.tv_sec - t1.tv_sec) * 1000 + (t2.tv_usec - t1.tv_usec) / 1000;
		etime = etime / 1000;

		printf("done, took %f sec. Verification...", etime);
		if (isSorted(ref, data, stSize)) {
			printf(" successful.\n");
		} else {
			printf(" FAILED.\n");
		}

		free(data);
		free(tmp);
		free(ref);
	}

	return EXIT_SUCCESS;
}