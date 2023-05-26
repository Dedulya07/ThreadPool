#pragma once

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <iostream>
#include <string>
#include <queue>

// ������ � ��������
#include <mutex>
#include <thread>
#include <condition_variable>
#include <atomic>

#include <functional>
#include <cassert>
#include <any>

#include "timer.h"

namespace MT {
	typedef unsigned long long int task_id;

	// ����������� ����� ������
	class Task {
		friend class ThreadPool;
	public:
		enum class TaskStatus {
			awaiting,
			completed
		};

		Task(const std::string& _description);

		// ����� ��� ������ ������� ���� �� ������� ������
		void send_signal();

		// ����������� �����, ������� ������ ���� ���������� �������������,
		// � ���� ���� ������� ������ ���������� ����� ������� ������� ������
		void virtual one_thread_method() = 0;

	protected:
		MT::Task::TaskStatus status;
		// ��������� �������� ������ (����� ��� ��������� �����������)
		std::string description;
		// ���������� ������������� ������
		MT::task_id id;

		MT::ThreadPool* thread_pool;

		// �����, ����������� �������
		void one_thread_pre_method();
	};

	// ������� ������� ��� std::thread ��� ����, ��� �� �����������
	// ��������� ������� ������
	struct Thread {
		std::thread _thread;
		std::atomic<bool> is_working;
	};

	class ThreadPool {

		friend void MT::Task::send_signal();

	public:
		ThreadPool(int count_of_threads);

		// ��������� ������� ���������� ������ � �������
		template <typename TaskChild>
		MT::task_id add_task(const TaskChild& task) {
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			task_queue.push(std::make_shared<TaskChild>(task));
			// ����������� ���������� id ����� ������
			// ����������� �������� id ����� 1
			task_queue.back()->id = ++last_task_id;
			// ��������� ������ � ������� �����
			task_queue.back()->thread_pool = this;
			tasks_access.notify_one();
			return last_task_id;
		}

		// �������� ������ ��������� ������� ������� ����� ��� ������������,
		// ���������� id ������, ������� ������ ������ ������ � 0 �����
		MT::task_id wait_signal();

		// �������� ������ ��������� ������� ������� �����, ��������� �����
		// ������� � ������������
		void wait();

		// ������������ ���������
		void stop();

		// ������������� ���������
		void start();

		// ��������� ���������� �� id
		template <typename TaskChild>
		std::shared_ptr<TaskChild> get_result(MT::task_id id) {
			auto elem = completed_tasks.find(id);
			if (elem != completed_tasks.end())
				return std::reinterpret_pointer_cast<TaskChild>(elem->second);
			else
				return nullptr;
		}

		// ������� ����������� �����
		void clear_completed();

		// ��������� ����� �����������
		void set_logger_flag(bool flag);

		~ThreadPool();

	private:
		// ��������, ����������� ������� ��� ����������������� ���������
		std::mutex task_queue_mutex;
		std::mutex completed_tasks_mutex;
		std::mutex signal_queue_mutex;

		// �������, ����������� ����� ��� ����������������� ������
		std::mutex logger_mutex;

		// �������, ����������� ������� ��������� ���������� (������ wait*)
		std::mutex wait_mutex;

		std::condition_variable tasks_access;
		std::condition_variable wait_access;

		// ����� ��������� �������
		std::vector<MT::Thread*> threads;

		// ������� �����
		std::queue<std::shared_ptr<Task>> task_queue;
		MT::task_id last_task_id;

		// ������ ����������� ����� � ���� ���-�������
		std::unordered_map<MT::task_id, std::shared_ptr<Task>> completed_tasks;
		unsigned long long completed_task_count;

		std::queue<task_id> signal_queue;

		// ���� ��������� ������ ����
		std::atomic<bool> stopped;
		// ���� ������������ ������
		std::atomic<bool> paused;
		std::atomic<bool> ignore_signals;
		// ����, ����������� �����������
		std::atomic<bool> logger_flag;

		Timer timer;

		// �������� �������, ���������������� ������ �����
		void run(MT::Thread* thread);

		// ������������ ��������� c �������� �������
		void receive_signal(MT::task_id id);

		// ���������� ������� ���������� ������
		bool run_allowed() const;

		// �������� ���������� ���� ����� �� �������
		bool is_comleted() const;

		// ��������, ��������� ���� �� ������ ������
		bool is_standby() const;
	};

	struct MassivePart {
		long long int begin;
		long long int size;
	};

	void separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts);
}

#endif