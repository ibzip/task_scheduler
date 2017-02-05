#include "ThreadPool.h"
ThreadPool::ThreadPool(int threads):stop(false) {
  // Launch "threads" no. of threads
  // Let them wait of condition.
  for (int i=0; i<threads; i++) {
    slaves.emplace_back(
      [this](){
        while (1) {
          std::function<void()> task;
          {
              std::unique_lock<std::mutex> lock(this->queue_mutex);
              this->condition.wait(lock, [this]{ return !this->tasks.empty() || this->stop; });

              if (this->stop)
                return;
              task = std::move(this->tasks.front());
              this->tasks.pop_front();
          }
          task();
        }
      }
    );
  }
}

ThreadPool::~ThreadPool() {
  queue_mutex.lock();
  stop = true;
  queue_mutex.unlock();

  // Notify all to exit
  condition.notify_all();

  for (auto it = slaves.begin(); it != slaves.end(); ++it) {
    it->join();
  }
}

