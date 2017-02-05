#include <chrono>
#include <functional>

using namespace std::chrono;

class Task {
  friend class Scheduler;
  private:
    std::function<void()> task;
    steady_clock::time_point start_time;
    steady_clock::duration repeat_after;
    int task_id;
  public:
    Task(std::function<void()>&& tsk, steady_clock::time_point start, steady_clock::duration rep_after, int tid);
    void execute_task() {
      task();
    }

    bool operator<(const Task& other) const;
};
