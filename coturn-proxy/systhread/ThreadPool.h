#ifndef SHARK_THREAD_THREADPOOL_H_
#define SHARK_THREAD_THREADPOOL_H_

#include <memory>
#include <vector>

#include "systhread/Worker.h"
#include "systhread/Scheduler.h"

namespace shark {

class ThreadPool {
 public:
  explicit ThreadPool(unsigned int num_workers);
  ~ThreadPool();

  std::shared_ptr<Worker> getLessUsedWorker();
  void start();
  void close();

 private:
  std::vector<std::shared_ptr<Worker>> workers_;
  std::shared_ptr<Scheduler> scheduler_;
};
}  // namespace shark

#endif  // SHARK_THREAD_THREADPOOL_H_
