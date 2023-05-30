#pragma once

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <iostream>
#include <string>
#include <queue>
#include <unordered_map>

// work with threads
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include "timer.h"

namespace MT {
	typedef unsigned long long int task_id;

	// abstract task class
	class Task {
		friend class ThreadPool;
	public:
		enum class TaskStatus {
			awaiting,
			completed
		};

		Task(const std::string& _description);

		// method for signaling the pool from the current task
		void send_signal();

		// abstract method that must be implemented by the user,
		// the body of this function must contain the path for solving the current task
		void virtual one_thread_method() = 0;

	protected:
		MT::Task::TaskStatus status;
		// text description of the task (needed for beautiful logging)
		std::string description;
		// unique task ID
		MT::task_id id;

		MT::ThreadPool* thread_pool;

		// thread-running method
		void one_thread_pre_method();
	};

	// simple wrapper over std::thread for keeping track
	// state of each thread
	struct Thread {
		std::thread _thread;
		std::atomic<bool> is_working;
	};

	class ThreadPool {

		friend void MT::Task::send_signal();

	public:
		ThreadPool(int count_of_threads);

		// template function for adding a task to the queue
		template <typename TaskChild>
		MT::task_id add_task(const TaskChild& task) {
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			task_queue.push(std::make_shared<TaskChild>(task));
			// assign a unique id to a new task
			// the minimum value of id is 1
			task_queue.back()->id = ++last_task_id;
			// associate a task with the current pool
			task_queue.back()->thread_pool = this;
			tasks_access.notify_one();
			return last_task_id;
		}

		// waiting for the current task queue to be completely processed or suspended,
		// returns the id of the task that first signaled and 0 otherwise
		MT::task_id wait_signal();

		// wait for the current task queue to be fully processed,
		// ignoring any pause signals
		void wait();

		// pause processing
		void stop();

		// resumption of processing
		void start();

		// get result by id
		template <typename TaskChild>
		std::shared_ptr<TaskChild> get_result(MT::task_id id) {
			auto elem = completed_tasks.find(id);
			if (elem != completed_tasks.end())
				return std::reinterpret_pointer_cast<TaskChild>(elem->second);
			else
				return nullptr;
		}

		// cleaning completed tasks
		void clear_completed();

		// setting the logging flag
		void set_logger_flag(bool flag);

		~ThreadPool();

	private:
		// mutexes blocking queues for thread-safe access
		std::mutex task_queue_mutex;
		std::mutex completed_tasks_mutex;
		std::mutex signal_queue_mutex;

		// mutex blocking serial output logger
		std::mutex logger_mutex;

		// mutex blocking functions waiting for results (wait* methods)
		std::mutex wait_mutex;

		std::condition_variable tasks_access;
		std::condition_variable wait_access;

		// set of available threads
		std::vector<MT::Thread*> threads;

		// task queue
		std::queue<std::shared_ptr<Task>> task_queue;
		MT::task_id last_task_id;

		// array of completed tasks in the form of a hash table
		std::unordered_map<MT::task_id, std::shared_ptr<Task>> completed_tasks;
		unsigned long long completed_task_count;

		std::queue<task_id> signal_queue;

		// pool stop flag
		std::atomic<bool> stopped;
		// pause flag
		std::atomic<bool> paused;
		std::atomic<bool> ignore_signals;
		// flag to enable logging
		std::atomic<bool> logger_flag;

		Timer timer;

		// main function that initializes each thread
		void run(MT::Thread* thread);

		// pause processing with signal emission
		void receive_signal(MT::task_id id);

		// permission to start the next thread
		bool run_allowed() const;

		// checking the execution of all tasks from the queue
		bool is_comleted() const;

		// checking if at least one thread is busy
		bool is_standby() const;
	};

	struct MassivePart {
		long long int begin;
		long long int size;
	};

	void separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts);
}

#endif