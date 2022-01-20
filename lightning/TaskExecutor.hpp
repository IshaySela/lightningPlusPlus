#include <functional>
#include <vector>
#include <thread>
#include <condition_variable>
#include <shared_mutex>

namespace lightning
{
    using Task = std::function<void()>;

    class TaskExecutor
    {
    public:
        TaskExecutor(const int thread_count);
        ~TaskExecutor();

        auto add_task(Task task) -> void;
        auto stop_all() -> void;

    private:
        static auto thread_worker(TaskExecutor &executor) -> void;

        //The amount of worker threads for this executor.
        const int thread_count;

        std::vector<std::thread> threads;
        std::vector<Task> tasks;
        std::mutex mutex;
        std::condition_variable cv;
        bool kill_threads = false;
    };
}