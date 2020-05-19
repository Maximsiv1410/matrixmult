#include "ThreadPool.hpp"
#include "Holders.hpp"

#include <iostream>
#include <numeric>
#include <iterator>
#include <fstream>
#include <string>
#include <algorithm>
#include <cmath>

#define FILENAME "output.txt"

// getting one element by sum of products
Result sum(Data& d) {
	long int res = std::inner_product(d.a.begin(), d.a.end(), d.b.begin(), 0);
	return Result(res, d.order);
}

// separating numbers for task-packages
std::vector<Data> separate(std::vector<long int>& lhs, std::vector<long int>& rhs) {
	std::vector<Data> data;
	int border = std::sqrt(lhs.size());
	Data temp;
	int counter = 0;
	for (int i = 0; i < border; i++)
		for (int j = 0; j < border; j++) {
			temp.order = counter++;
			for (int k = 0; k < border; k++) {
				temp.a.push_back(lhs[i*border + k]);
			}

			for (int k = 0; k < border; k++) {
				temp.b.push_back(rhs[k*border + j]);
			}

			data.push_back(std::move(temp));
			temp.a.clear();
			temp.b.clear();
		}
	return data;
}



int main(int argc, char* argv[]) {
	if (argc < 5) {
		std::cout << "usage: <prog-name> <thread-amount> <file1.txt> <file2.txt>  <output.txt>\n";
		return 1;
	}

	// simple check to drop overhead
	int threads = std::stoi(argv[1]);
	threads = threads < 50 ?  threads : 50;

	ThreadPool pool(threads);

	std::ifstream first(argv[2]);
	std::vector<long int> A{ std::istream_iterator<long int>(first), std::istream_iterator<long int>() };

	std::ifstream second(argv[3]);
	std::vector<long int> B{ std::istream_iterator<long int>(second), std::istream_iterator<long int>() };

	// separating data
	int border = std::sqrt(A.size());
	std::vector<Data> dataVec = separate(A, B);


	// setting futures to get them later
	std::vector<std::future<Result>> futures;
	for (auto&& task : dataVec) {
		futures.emplace_back(pool.push(sum, task));
	}

	auto start = std::chrono::high_resolution_clock::now();
	pool.start();

	// getting results
	std::vector<Result> vecres;
	for (auto&& res : futures) {
		vecres.push_back(std::move(res.get()));
	}

	// sorting by order to output correctly
	std::sort(vecres.begin(), vecres.end(), [](auto lhs, auto rhs) { return lhs.order < rhs.order;  });
	//std::sort(vecres.begin(), vecres.end(), std::less<Result>());


	

	// opening file to output
	std::ofstream out(argv[4]);

	for (int i = 0; i < border; i++) {
		for (int j = 0; j < border; j++) {
			out << vecres[i*border + j].result << " ";
			if (j == border - 1) {
				out << "\n";
			}
		}
	}

}