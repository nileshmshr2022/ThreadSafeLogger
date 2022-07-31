
#ifndef ACTIVE_H_
#define ACTIVE_H_

#include <thread>
#include <functional>
#include <condition_variable>
#include <mutex>
#include <memory>

#include "shared_queue.h"

namespace Utility {
	typedef std::function<void()> Callback;

	class Active {
	private:
		Active(const Active&) = delete;
		Active& operator=(const Active&) = delete;

		Active();

		void doDone() { done_ = true; }
		void run();
		shared_queue<Callback> mq_;
		std::thread thd_;
		bool done_;

	public:
		virtual ~Active();
		void send(Callback msg_);
		static std::unique_ptr<Active> createActive();
	};
}

#endif
