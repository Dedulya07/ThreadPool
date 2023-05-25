#include "thread_pool.h"

struct TestTask : public MT::Task {
	int duration;
	TestTask(const std::string& id, int _duration) : Task(id) {
		duration = _duration + ((rand() % 10) + 1);
	};
	void one_thread_method() override {
		if (duration >= 10) {
			signal_thread_pool();
			return;
		}
		else
			std::this_thread::sleep_for(std::chrono::seconds(duration));
	}
};


int main() {

	MT::task_id signal;
	MT::ThreadPool thread_pool(3);
	thread_pool.set_logger_flag(true);
	thread_pool.start();
	for (int i = 0; i < 20; i++) {
		thread_pool.add_task(TestTask("TestTask_" + std::to_string(i + 1), i));
	}
	std::this_thread::sleep_for(std::chrono::seconds(5));
	signal = thread_pool.wait_signal();
	if (signal != 0) {
		auto result = thread_pool.get_result<TestTask>(signal);
		std::cout << signal << " " << result->duration << std::endl;
	}

	thread_pool.clear_completed();
	signal = thread_pool.wait_signal();
	if (signal != 0) {
		auto result = thread_pool.get_result<TestTask>(signal);
		std::cout << signal << " " << result->duration << std::endl;
	}

	thread_pool.wait();

	signal = thread_pool.wait_signal();
	if (signal != 0) {
		auto result = thread_pool.get_result<TestTask>(signal);
		std::cout << signal << " " << result->duration << std::endl;
	}

	return 0;
}