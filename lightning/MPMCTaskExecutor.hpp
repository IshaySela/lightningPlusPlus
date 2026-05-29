#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include "moodycamel/concurrentqueue.h"

namespace lightning
{
    template <typename T>
    concept Task = (std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>)
                   && requires(T t) { t.operator()(); };

    // Lock-free task executor.
    //
    // The task queue is moodycamel::ConcurrentQueue — no mutex, no semaphore.
    // Workers spin on try_dequeue() and yield when the queue is empty, avoiding
    // semaphore syscall overhead at the cost of CPU burn during idle periods.
    //
    // Shutdown sets kill_threads; workers exit after draining the queue.
    //
    // Tasks are stored as std::unique_ptr<T> so T need not be default-
    // constructible or move-assignable.
    template <typename T = std::function<void()>>
    requires Task<T>
    class TaskExecutor
    {
    public:
        explicit TaskExecutor(int thread_count) : thread_count(thread_count)
        {
            if (thread_count < 1)
                throw std::invalid_argument("TaskExecutor: at least 1 thread is required.");

            threads.reserve(thread_count);
            for (int i = 0; i < thread_count; i++)
                threads.emplace_back(thread_worker, std::ref(*this));
        }

        ~TaskExecutor() { stop_all(); }

        // Not copyable or movable — threads hold a reference to *this.
        TaskExecutor(const TaskExecutor&)            = delete;
        TaskExecutor& operator=(const TaskExecutor&) = delete;
        TaskExecutor(TaskExecutor&&)                 = delete;
        TaskExecutor& operator=(TaskExecutor&&)      = delete;

        auto add_task(T task) -> void
        {
            tasks.enqueue(std::make_unique<T>(std::move(task)));
        }

        auto stop_all() -> void
        {
            if (kill_threads.exchange(true, std::memory_order_acq_rel))
                return;

            for (auto& t : threads)
                if (t.joinable()) t.join();
        }

    private:
        static auto thread_worker(TaskExecutor& executor) -> void
        {
            std::unique_ptr<T> task;

            while (true)
            {
                if (executor.tasks.try_dequeue(task))
                {
                    (*task)();
                }
                else if (executor.kill_threads.load(std::memory_order_acquire))
                {
                    while (executor.tasks.try_dequeue(task))
                        (*task)();
                    break;
                }
                else
                {
                    std::this_thread::yield();
                }
            }
        }

        const int thread_count;
        std::vector<std::thread> threads;
        moodycamel::ConcurrentQueue<std::unique_ptr<T>> tasks;
        std::atomic<bool> kill_threads{false};
    };
} // namespace lightning
