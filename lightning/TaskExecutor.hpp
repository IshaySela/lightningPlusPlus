#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <concepts>

namespace lightning
{
    // using Task = std::function<void()>;

    template <typename T>
    concept Task = requires(T t)
    {
        {
            t.operator()()};
    };

    template <typename T = std::function<void()>>
    requires Task<T>
    class TaskExecutor
    {
    public:
        /**
         * @brief Construct a new Task Executor object and the worker thread.
         * 
         * @param thread_count The amount of worker threads to create.
         */
        TaskExecutor(const int thread_count) : thread_count(thread_count)
        {
            if (thread_count < 1)
                throw std::invalid_argument("Invalid thread count. at least 1 thread is needed.");

            for (int i = 0; i < thread_count; i++)
            {
                this->threads.push_back(std::thread(TaskExecutor::thread_worker, std::ref(*this)));
            }
        }

        /**
         * @brief Wait for all of the threads to finish their tasks.
         */
        ~TaskExecutor()
        {
            for (auto &t : this->threads)
            {
                t.join();
            }
        }

        /**
         * @brief Push the task to the tasks vector.
         * 
         * @param task The task to push.
         * @param deleteAfter true if delete should be called on the task after it was called. 
         * this option is provided for abstract classes or non-copyable data structures that need
         * to be heap allocated.
         */
        auto add_task(T *task, bool deleteAfter = false) -> void
        {
            mutex.lock();
            this->tasks.push_back({task, deleteAfter});
            mutex.unlock();

            this->cv.notify_one();
        }

        auto stop_all() -> void;

    private:
        static auto thread_worker(TaskExecutor<T> &executor) -> void
        {
            bool die = false;

            while (!die)
            {
                std::unique_lock<std::mutex> lock(executor.mutex);
                executor.cv.wait(lock, [&executor]
                                 { return executor.tasks.size() > 0 || executor.kill_threads; });

                die = executor.kill_threads;

                auto [task, shouldDelete] = executor.tasks.front();
                executor.tasks.erase(executor.tasks.begin());

                lock.unlock();
                executor.cv.notify_one();

                (*task)();

                if(shouldDelete)
                    delete task;
            }
        }

        //The amount of worker threads for this executor.
        const int thread_count;

        std::vector<std::thread> threads;

        using DeleteAfter = bool;
        std::vector<std::pair<T*, DeleteAfter>> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        bool kill_threads = false;
    };
}