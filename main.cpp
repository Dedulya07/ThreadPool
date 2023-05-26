#include "thread_pool.h"

struct TestTask : public MT::Task {
	int duration;
	TestTask(const std::string& id, int _duration) : Task(id) {
		duration = _duration + ((rand() % 20));
		description += "_" + std::to_string(duration);
	};
	void one_thread_method() override {
		if (duration >= 10) {
			send_signal();
			return;
		}
		else
			std::this_thread::sleep_for(std::chrono::seconds(duration));
	}
};

int main() {

	MT::ThreadPool thread_pool(3);
	thread_pool.set_logger_flag(true);
	// снимаем пул с паузы, позволяя потокам браться за задачи налету
	thread_pool.start();
	for (int i = 0; i < 20; i++) {
		thread_pool.add_task(TestTask("TestTask_" + std::to_string(i + 1), 1));
	}
	// дождемся трех "редких успехов"
	for (int i = 0; i < 3; i++) {
		MT::task_id signal = thread_pool.wait_signal();
		if (signal != 0) {
			auto result = thread_pool.get_result<TestTask>(signal);
			std::cout << "id: " << signal << ", duration: " << result->duration << std::endl;
		}
	}

	return 0;
}