#include "thread_pool.h"

struct SumTask : public MT::Task {
    SumTask(const std::string& id) : Task(id) {
    };
    void one_thread_method() override {
        char mas[1024];
        memset(mas, 0, 1024);
        for (unsigned long long int i = 0; i < 8000000 - 1; i++) {
            char* d = new char[1024];
            memcpy(d, mas, 1024);
            delete[] d;
        }
    }
};

int main() {

    MT::ThreadPool thread_pool(10);
    thread_pool.set_logger_flag(true);
    // remove the pool from a pause, allowing streams to take on the tasks on the fly
    thread_pool.start();
    for (int i = 0; i < 20; i++) {
        thread_pool.add_task(SumTask("SumTask_" + std::to_string(i + 1)));
    }
    // we will wait for three "rare successes"
    for (int i = 0; i < 3; i++) {
        MT::task_id signal = thread_pool.wait_signal();
        if (signal != 0) {
            auto result = thread_pool.get_result<SumTask>(signal);
            std::cout << "id: " << signal << std::endl;
        }
    }

    return 0;
}