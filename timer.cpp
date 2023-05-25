#include "timer.h"

Timer::Timer() {
	current_checkpoint = std::chrono::system_clock::now();
	start_time = current_checkpoint;
}

std::string Timer::make_checkpoint(const std::string& message) {
	std::chrono::system_clock::time_point now_time = std::chrono::system_clock::now();

	std::stringstream ss;
	ss <<
		"ThreadId: " << std::setw(5) << std::this_thread::get_id() <<
		" ::: Message: " << std::setw(30) << message <<
		" ::: Time: ";
	print_time(now_time, ss);
	ss << " ::: Duration (h:m:s:ms): ";
	Duration(now_time - current_checkpoint).print(ss);
	ss << ", total: ";
	Duration(now_time - start_time).print(ss);

	current_checkpoint = now_time;
	return ss.str();
}

void Timer::print_time(std::chrono::system_clock::time_point time_point, std::basic_iostream<char>& stream) const {
	time_t time = std::chrono::system_clock::to_time_t(time_point);
	tm* time_tm = localtime(&time);
	stream << std::put_time(time_tm, "%d.%m.%Y-%T");
}

Timer::Duration::Duration(const std::chrono::duration<double>& d) {
	std::chrono::duration<double> duration = d;
	hours = std::chrono::duration_cast<std::chrono::hours>(duration);
	duration -= hours;
	minutes = std::chrono::duration_cast<std::chrono::minutes>(duration);
	duration -= minutes;
	seconds = std::chrono::duration_cast<std::chrono::seconds>(duration);
	duration -= seconds;
	milliseconds = std::chrono::duration_cast<std::chrono::milliseconds>(duration);
}

void Timer::Duration::print(std::basic_iostream<char>& stream) const {
	stream.fill('0');
	stream <<
		std::setw(2) << hours.count() << ":" <<
		std::setw(2) << minutes.count() << ":" <<
		std::setw(2) << seconds.count() << ":" <<
		std::setw(3) << milliseconds.count();
	stream.fill();
}
