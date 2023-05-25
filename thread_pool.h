#pragma once

#ifndef _THREAD_POOL_H_
#define _THREAD_POOL_H_

#include <vector>
#include <iostream>
#include <string>
#include <queue>

// работа с потоками
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

	// абстрактный класс задачи
	class Task {
		friend class ThreadPool;
	public:
		enum class TaskStatus {
			awaiting,
			completed
		};

		Task(const std::string& _description);

		// метод для подачи сигнала пулу из текущей задачи
		void signal_thread_pool();

		// абстрактный метод, который должен быть реализован пользователем,
		// в теле этой функции должен находиться тракт решения текущей задачи
		void virtual one_thread_method() = 0;

	protected:
		MT::Task::TaskStatus status;
		std::string description;
		MT::task_id id;

		MT::ThreadPool* thread_pool;

		// метод, запускаемый потоком
		void one_thread_pre_method();
	};

	// простая обертка над std::thread для того, что бы отслеживать состояние каждого потока
	struct Thread {
		std::thread _thread;
		std::atomic<bool> is_working;
	};

	class ThreadPool {

		friend void MT::Task::signal_thread_pool();

	public:
		ThreadPool(int count_of_threads);

		template <typename TaskChild>
		MT::task_id add_task(const TaskChild& task) {
			std::lock_guard<std::mutex> lock(task_queue_mutex);
			task_queue.push(std::make_shared<TaskChild>(task));
			task_queue.back()->id = ++last_task_id;
			task_queue.back()->thread_pool = this;
			tasks_access.notify_one();
			return last_task_id;
		}

		// ожидание полной обработки текущей очереди задач или приостановки,
		// возвращает id задачи, которая первая подавала сигнал и 0 иначе
		MT::task_id wait_signal();

		// ожидание полной обработки текущей очереди задач, игнорируя любые
		// сигналы о приостановке
		void wait();

		// приостановка обработки
		void stop();

		// возобновление обработки
		void start();

		// получение результата по id
		template <typename ResultType>
		std::shared_ptr<ResultType> get_result(MT::task_id id) {
			auto elem = completed_tasks.find(id);
			if (elem != completed_tasks.end())
				return std::reinterpret_pointer_cast<ResultType>(elem->second);
			else
				return nullptr;
		}

		// очистка завершенных задач
		void clear_completed();

		// установка флага логирования
		void set_logger_flag(bool flag);

		~ThreadPool();

	private:
		// mutex'ы, блокирующие очереди для потокобезопасного изменения
		std::mutex task_queue_mutex;
		std::mutex completed_tasks_mutex;
		std::mutex signal_queue_mutex;

		// mutex, блокирующий логер для последовательного вывода
		std::mutex logger_mutex;

		// mutex, блокирующий функции ожидающие результаты (методы wait*)
		std::mutex wait_mutex;

		std::condition_variable tasks_access;
		std::condition_variable wait_access;

		// набор доступных потоков
		std::vector<MT::Thread*> threads;

		// очередь задач
		std::queue<std::shared_ptr<Task>> task_queue;
		MT::task_id last_task_id;

		// массив выполненных задач
		std::unordered_map<MT::task_id, std::shared_ptr<Task>> completed_tasks;
		unsigned long long completed_task_count;

		std::queue<task_id> signal_queue;

		// флаг остановки работы пула
		std::atomic<bool> stopped;
		// флаг приостановки работы
		std::atomic<bool> paused;
		std::atomic<bool> ignore_signals;
		// флаг, разрешающий логирования
		std::atomic<bool> logger_flag;

		Timer timer;

		// основная функция, инициализирующая каждый поток
		void run(MT::Thread* thread);

		// приостановка обработки c выбросом сигнала
		void stop(MT::task_id id);

		// разрешение запуска очередного потока
		bool run_allowed() const;

		// проверка выполнения всех задач из очереди
		bool is_comleted() const;

		// проверка, занятости хотя бы одного потока
		bool is_standby() const;
	};

	struct MassivePart {
		long long int begin;
		long long int size;
	};

	void separate_massive(long long int full_size, long long int part_size, int thread_count, std::vector<MassivePart>& parts);
}

#endif