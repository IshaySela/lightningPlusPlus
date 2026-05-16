#pragma once
#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <shared_mutex>
#include <concepts>

namespace lightning
{

    template <typename T>
    concept CopyConstructibleOrAssignable = std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>;
    // To use std::vector::push_back with T, T must be either copy constructible or move assignable.
    template <typename T>
    concept Task = CopyConstructibleOrAssignable<T> && requires(T t)
    {
        {
            t.operator()()
        };
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
            this->stop_all();
        }

        /**
         * @brief Push the task to the tasks vector.
         *
         * @param task The task to push.
         */
        auto add_task(T task) -> void
        {
            mutex.lock();

            if constexpr (std::is_copy_constructible<T>::value)
                this->tasks.push_back(task);
            else if (std::is_move_assignable<T>::value)
                this->tasks.push_back(std::move(task));

            mutex.unlock();

            this->cv.notify_one();
        }

        auto stop_all() -> void
        {
            this->kill_threads = true;
            this->cv.notify_all();

            for (auto &t : this->threads)
            {
                t.join();
            }
        }

    private:
        static auto thread_worker(TaskExecutor<T> &executor) -> void
        {
            bool die = false;

            while (true)
            {
                std::unique_lock<std::mutex> lock(executor.mutex);
                executor.cv.wait(lock, [&executor]
                                 { return executor.tasks.size() > 0 || executor.kill_threads; });

                die = executor.kill_threads;

                if (die)
                    break;

                // This is a touch complex.
                // TODO: Find more elegant solution.
                if constexpr (std::is_copy_constructible<T>::value)
                {
                    auto task = executor.tasks.front();
                    executor.tasks.erase(executor.tasks.begin());

                    lock.unlock();
                    executor.cv.notify_one();

                    task();
                }
                else
                {
                    auto task = std::move(executor.tasks.front());
                    executor.tasks.erase(executor.tasks.begin());

                    lock.unlock();
                    executor.cv.notify_one();

                    task();
                }
            }
        }

        // The amount of worker threads for this executor.
        const int thread_count;

        std::vector<std::thread> threads;

        using DeleteAfter = bool;
        std::vector<T> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        bool kill_threads = false;
    };
}