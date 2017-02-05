#include <functional>
#include <deque>
#include <mutex>
#include <condition_variable>
#include <vector>
#include <thread>

class Scheduler;
class ThreadPool {
  friend class Scheduler;
  private:
    // mutex and condition var for syncing dequeue access
    std::mutex queue_mutex;
    std::condition_variable condition;

    // contains tasks
    std::deque<std::function<void()>> tasks;

    // vector of slave threads
    std::vector<std::thread> slaves;
    bool stop;
  public:
    ThreadPool(int threads);
    ~ThreadPool();
};
