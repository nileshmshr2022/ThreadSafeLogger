
#include "active.h"
#include <cassert>

using namespace Utility;

Active::Active() : done_(false) {}

Active::~Active() {
	Callback quit_token = std::bind(&Active::doDone, this);
	send(quit_token);
	thd_.join();
}


void Active::send(Callback msg_) {
	mq_.push(msg_);
}


void Active::run() {
	while (!done_) {
		Callback func;
		mq_.wait_and_pop(func);
		func();
	}
}

std::unique_ptr<Active> Active::createActive() {
	std::unique_ptr<Active> aPtr(new Active());
	aPtr->thd_ = std::thread(&Active::run, aPtr.get());
	return aPtr;
}
