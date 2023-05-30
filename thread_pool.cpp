#include "thread_pool.h"

MT::Task::Task(const std::string& _description) {
	description = _description;
	id = 0;
	status = MT::Task::TaskStatus::awaiting;
	thread_pool = nullptr;
}

void MT::Task::send_signal() {
	thread_pool->receive_signal(id);
}

void MT::Task::one_thread_pre_method() {
	one_thread_method();
	status = MT::Task::TaskStatus::completed;
}

MT::ThreadPool::ThreadPool(int count_of_threads) {
	stopped = false;
	paused = true;
	logger_flag = false;
	last_task_id = 0;
	completed_task_count = 0;
	ignore_signals = true;
	for (int i = 0; i < count_of_threads; i++) {
		MT::Thread* th = new MT::Thread;
		th->_thread = std::thread{ &ThreadPool::run, this, th };
		th->is_working = false;
		threads.push_back(th);
	}
}

MT::ThreadPool::~ThreadPool() {
	stopped = true;
	tasks_access.notify_all();
	for (auto& thread : threads) {
		thread->_thread.join();
		delete thread;
	}
}

bool MT::ThreadPool::run_allowed() const {
	return (!task_queue.empty() && !paused);
}

void MT::ThreadPool::run(MT::Thread* _thread) {
	while (!stopped) {
		std::unique_lock<std::mutex> lock(task_queue_mutex);

		// the current thread is in standby mode in case
		// if there are no jobs or the entire pool is suspended
		_thread->is_working = false;
		tasks_access.wait(lock, [this]()->bool { return run_allowed() || stopped; });
		_thread->is_working = true;

		if (run_allowed()) {
			// a thread takes a task from the queue
			auto elem = std::move(task_queue.front());
			task_queue.pop();
			lock.unlock();

			if (logger_flag) {
				std::lock_guard<std::mutex> lg(logger_mutex);
				std::cout << timer.make_checkpoint("Run task " + elem->description) << std::endl;
			}
			// solving task
			elem->one_thread_pre_method();
			if (logger_flag) {
				std::lock_guard<std::mutex> lg(logger_mutex);
				std::cout << timer.make_checkpoint("End task " + elem->description) << std::endl;
			}

			// saving the result
			std::lock_guard<std::mutex> lg(completed_tasks_mutex);
			completed_tasks[elem->id] = elem;
			completed_task_count++;
		}
		// wake up threads that are waiting on the pool (wait* methods)
		wait_access.notify_all();
	}
}

void MT::ThreadPool::start() {
	if (paused) {
		paused = false;
		// give all threads a permission signal to access
		// to the tasks queue
		tasks_access.notify_all();
	}
}

void MT::ThreadPool::stop() {
	paused = true;
}

void MT::ThreadPool::receive_signal(MT::task_id id) {
	std::lock_guard<std::mutex> lock(signal_queue_mutex);
	signal_queue.emplace(id);
	if (!ignore_signals)
		stop();
}

void MT::ThreadPool::wait() {
	std::lock_guard<std::mutex> lock_wait(wait_mutex);

	start();

	std::unique_lock<std::mutex> lock(task_queue_mutex);
	wait_access.wait(lock, [this]()->bool { return is_comleted(); });

	stop();
}

MT::task_id MT::ThreadPool::wait_signal() {
	std::lock_guard<std::mutex> lock_wait(wait_mutex);

	ignore_signals = false;

	signal_queue_mutex.lock();
	if (signal_queue.empty())
		start();
	else
		stop();
	signal_queue_mutex.unlock();

	std::unique_lock<std::mutex> lock(task_queue_mutex);
	wait_access.wait(lock, [this]()->bool { return is_comleted() || is_standby(); });

	ignore_signals = true;

	// at the moment all tasks by id from
	// signal_queues are considered completed
	std::lock_guard<std::mutex> lock_signals(signal_queue_mutex);
	if (signal_queue.empty())
		return 0;
	else {
		MT::task_id signal = std::move(signal_queue.front());
		signal_queue.pop();
		return signal;
	}
}

bool MT::ThreadPool::is_comleted() const {
	return completed_task_count == last_task_id;
}

bool MT::ThreadPool::is_standby() const {
	if (!paused)
		return false;
	for (const auto& thread : threads)
		if (thread->is_working)
			return false;
	return true;
}

void MT::ThreadPool::clear_completed() {
	std::scoped_lock lock(completed_tasks_mutex, signal_queue_mutex);
	completed_tasks.clear();
	while (!signal_queue.empty())
		signal_queue.pop();
}

void MT::ThreadPool::set_logger_flag(bool flag) {
	logger_flag = flag;
}

void MT::separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts) {
	parts.clear();
	long long int full_parts = full_size / part_size;
	long long int last_pos = 0;
	if (full_parts > 0) {
		parts.resize(full_parts);
		for (auto& part : parts) {
			part.begin = last_pos;
			part.size = part_size;
			last_pos += part_size;
		}
	}
	// divide the remainder into smaller pieces between all threads
	long long int remains = full_size % part_size;
	if (remains == 0)
		return;
	long long int each_thread = remains / thread_count;
	if (each_thread == 0) {
		parts.push_back(MassivePart{ last_pos, remains });
		return;
	}
	for (int i = 0; i < thread_count; i++) {
		parts.push_back(MassivePart{ last_pos, each_thread });
		last_pos += each_thread;
	}
	if (remains % thread_count > 0)
		parts.push_back(MassivePart{ last_pos, remains % thread_count });
}
