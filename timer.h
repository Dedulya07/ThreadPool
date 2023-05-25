#pragma once
#ifndef _TIMER_H_
#define _TIMER_H_

#include <iostream>
#include <chrono>
#include <iomanip>
#include <thread>
#include <string>
#include <sstream>

class Timer {
public:
	Timer();
	std::string make_checkpoint(const std::string& message="<none>");
private:
	class Duration {
	public:
		Duration(const std::chrono::duration<double>& d);
		void print(std::basic_iostream<char>& stream) const;
	private:
		std::chrono::hours hours;
		std::chrono::minutes minutes;
		std::chrono::seconds seconds;
		std::chrono::milliseconds milliseconds;
	};

	std::chrono::system_clock::time_point current_checkpoint;
	std::chrono::system_clock::time_point start_time;
	void print_time(std::chrono::system_clock::time_point time_point, std::basic_iostream<char>& stream) const;
};

#endif