#pragma once
#include <atomic>
#include <functional>
#include <memory>
#include <stdexcept>
#include <thread>
#include <vector>
#include "blockingconcurrentqueue.h"

namespace lightning
{
    template <typename T>
    concept Task = (std::is_copy_constructible_v<T> || std::is_move_constructible_v<T>)
                   && requires(T t) { t.operator()(); };

    // Lock-free multi-producer / multi-consumer task executor.
    //
    // The task queue is moodycamel::BlockingConcurrentQueue — no mutex on the
    // hot path. Workers block inside wait_dequeue() using moodycamel's built-in
    // lightweight semaphore; no external counting_semaphore is needed and the
    // sched_yield spin-loop from try_dequeue is eliminated entirely.
    //
    // Shutdown uses a null-pointer sentinel: stop_all() enqueues one nullptr per
    // worker thread; each worker exits when it dequeues a null task.
    //
    // Tasks are stored as std::unique_ptr<T> so T need not be default-
    // constructible or move-assignable (wait_dequeue requires move-assignment of
    // the stored element type; unique_ptr satisfies this regardless of T's own
    // assignment operators).
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

        // Enqueue a task and wake one sleeping worker.
        // Thread-safe; may be called from any thread simultaneously.
        auto add_task(T task) -> void
        {
            tasks.enqueue(std::make_unique<T>(std::move(task)));
        }

        // Signal all workers to stop, then block until they have all exited.
        // Idempotent — safe to call from the destructor even after an explicit call.
        auto stop_all() -> void
        {
            if (kill_threads.exchange(true, std::memory_order_acq_rel))
                return; // already stopped

            // Enqueue one null sentinel per worker — each null wakes and exits
            // exactly one thread blocked in wait_dequeue().
            for (int i = 0; i < thread_count; i++)
                tasks.enqueue(nullptr);

            for (auto& t : threads)
                if (t.joinable()) t.join();
        }

    private:
        static auto thread_worker(TaskExecutor& executor) -> void
        {
            std::unique_ptr<T> task;

            while (true)
            {
                executor.tasks.wait_dequeue(task);

                if (!task) // null sentinel → shutdown
                    break;

                (*task)();
            }
        }

        const int thread_count;
        std::vector<std::thread> threads;
        moodycamel::BlockingConcurrentQueue<std::unique_ptr<T>> tasks;
        std::atomic<bool> kill_threads{false};
    };
} // namespace lightning
