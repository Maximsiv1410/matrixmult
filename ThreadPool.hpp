#ifndef __MY_THREAD_POOL__
#define __MY_THREAD_POOL__

#include <thread>
#include <future>
#include <condition_variable>
#include <mutex>
#include <vector>
#include <queue>
#include <atomic>
#include <type_traits>
#include <functional>
#include <iostream>

class ThreadPool {
public:
	
	ThreadPool(size_t size) : m_size(size) {
		workers.reserve(size);
		for (int i = 0; i < size; ++i) {
			workers.emplace_back(std::bind(&ThreadPool::entry, this, i));
		}
	}

	ThreadPool() {
		size_t sz = std::thread::hardware_concurrency();
		if (!sz) sz = 2;
		workers.reserve(sz);
		for (int i = 0; i < sz; ++i) {
			workers.emplace_back(std::bind(&ThreadPool::entry, this, i));
		}
	}

	ThreadPool(int size, std::queue<std::function<void()>>&& labor) :m_size(size) {
		for (int i = 0; i < size; ++i) {
			workers.emplace_back(std::bind(&ThreadPool::entry, this, i));
		}

		{ 
			std::lock_guard<std::mutex> locker(mtx);
			tasks = std::move(labor);
			condVar.notify_all();
		}

	}

	~ThreadPool() {
		{
			std::unique_lock<std::mutex> lock(mtx);
			status = false;
			condVar.notify_all();
		}
		for (auto&& thread : workers) {
			thread.join();
		}
	}


	//deferred execution
	template<typename Fn, typename... Args>
	decltype(auto) push(Fn&& fn, Args&&... args) {
		typedef typename std::result_of<Fn(Args...)>::type ret;

		auto task = std::make_shared<std::packaged_task<ret()>>
			(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));

		std::future<ret> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(mtx);

			tasks.emplace([task]() { (*task)(); });
		}
		return res;
	}

	// immediate execution
	template<typename Fn, typename... Args>
	decltype(auto) execute(Fn&& fn, Args&&... args) {
		typedef typename std::result_of<Fn(Args...)>::type ret;

		auto task = std::make_shared<std::packaged_task<ret()>>
			(std::bind(std::forward<Fn>(fn), std::forward<Args>(args)...));

		std::future<ret> res = task->get_future();
		{
			std::unique_lock<std::mutex> lock(mtx);

			tasks.emplace([task]() { (*task)(); });
		}
		condVar.notify_one(); 
		return res;
	}


	void start() {
		std::unique_lock<std::mutex> lock(mtx);
		if (status) {
			condVar.notify_all();
		}
		else {
			throw std::runtime_error("ThreadPool is stopped");
		}
	}

	/*void execute(std::function<void()> task) {
		std::unique_lock<std::mutex> lock(mtx);
		tasks.push(std::move(task));
		condVar.notify_one();
	}*/

	void entry(int id) {
		while (true) {
			std::function<void()> job;
			{
				std::unique_lock<std::mutex> lock(mtx);
				while (status && tasks.empty()) {
					condVar.wait(lock);
				}
				if (tasks.empty()) {
					return;
				}
				job = std::move(tasks.front());
				tasks.pop();
			}
			job();
		}
	}

private:
	size_t m_size;
	std::vector<std::thread> workers;
	std::queue<std::function<void()>> tasks;
	//std::atomic<bool> status{ true };
	bool status{ true };
	std::condition_variable condVar;
	std::mutex mtx;
};

#endif