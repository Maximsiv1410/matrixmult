#ifndef __MY_HOLDERS__
#define __MY_HOLDERS__

#include <vector>
#include <type_traits>

/* 
	The thing is to mark concrete order of Data Package to sort them later as they should be
*/
class Data {
public:
	Data() {}
	Data(std::vector<long int> first, std::vector<long int> second, int order) {
		a = std::move(first);
		b = std::move(second);
		this->order = order;
	}

	Data(const Data& other) {
		this->a = other.a;
		this->b = other.b;
		this->order = other.order;
	}

	Data(Data&& other) {
		this->a = std::move(other.a);
		this->b = std::move(other.b);
		this->order = other.order;
		other.order = -1;
	}

	std::vector<long int> a;
	std::vector<long int> b;
	int order;
};

class Result {
public:
	Result() : result(-1), order(-1) {}
	Result(const Result& other) : result(other.result), order(other.order) {}
	Result(int result, int order) : result(result), order(order) {}

	Result& operator=(const Result& other) {
		this->result = other.result;
		this->order = other.order;
		return *this;
	}

	bool operator < (const Result& other) const {
		return this->order < other.order;
	}

	long int result;
	int order;
};



template<typename Fn, typename... Args>
struct TypeHolder {
	typedef std::result_of_t<Fn(Args...)> ret_type;
};

#endif