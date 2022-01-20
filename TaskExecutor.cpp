#include "lightning/TaskExecutor.hpp"

namespace lightning {
    TaskExecutor::TaskExecutor(const int thread_count) : thread_count(thread_count) {
        if (thread_count < 1)
            throw std::invalid_argument("Invalid thread count. at least 1 thread is needed.");


        for (int i = 0; i < thread_count; i++) {
            this->threads.push_back(std::thread(TaskExecutor::thread_worker, std::ref(*this)));
        }
    }

    auto TaskExecutor::add_task(lightning::Task task) -> void {
        mutex.lock();
        this->tasks.push_back(task);
        mutex.unlock();

        this->cv.notify_one();
    }

    auto TaskExecutor::thread_worker(TaskExecutor &executor) -> void{
        bool die = false;


        while (!die) {
            std::unique_lock<std::mutex> lock(executor.mutex);
            executor.cv.wait(lock, [&executor] { return executor.tasks.size() > 0 || executor.kill_threads; });

            die = executor.kill_threads;

            auto task = executor.tasks.front();
            executor.tasks.erase(executor.tasks.begin());

            lock.unlock();
            executor.cv.notify_one();

            task();
        }
    }

    TaskExecutor::~TaskExecutor() {

        for (auto &t : this->threads) {
            t.join();
        }
    }

    auto TaskExecutor::stop_all() -> void {
        this->kill_threads = true;
    }
}
