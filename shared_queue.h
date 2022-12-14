#ifndef SHARED_QUEUE
#define SHARED_QUEUE

#include <queue>
#include <mutex>
#include <exception>
#include <condition_variable>

template<typename T>
class shared_queue
{
	std::queue<T> queue_;
	mutable std::mutex m_;
	std::condition_variable data_cond_;

	shared_queue& operator=(const shared_queue&) = delete;
	shared_queue(const shared_queue& other) = delete;

public:
	shared_queue() {}

	void push(T item) {
		std::lock_guard<std::mutex> lock(m_);
		queue_.push(item);
		data_cond_.notify_one();
	}

	bool try_and_pop(T& popped_item) {
		std::lock_guard<std::mutex> lock(m_);
		if (queue_.empty()) {
			return false;
		}
		popped_item = queue_.front();
		queue_.pop();
		return true;
	}

	void wait_and_pop(T& popped_item) {
		std::unique_lock<std::mutex> lock(m_);
		while (queue_.empty())
		{
			data_cond_.wait(lock);
		}
		popped_item = queue_.front();
		queue_.pop();
	}

	bool empty() const {
		std::lock_guard<std::mutex> lock(m_);
		return queue_.empty();
	}

	unsigned size() const {
		std::lock_guard<std::mutex> lock(m_);
		return queue_.size();
	}
};

#endif
