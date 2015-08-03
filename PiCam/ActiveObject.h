#pragma once

#include <functional>
#include <deque>
#include <mutex>
#include <thread>
#include <condition_variable>

template< typename T >
class ActiveObject
{
public:
  ActiveObject(T &&iInternal)
    : mInternal(std::move(iInternal))
  {
  }

  ~ActiveObject()
  {
    stop();
  }

  inline void run()
  {
    workHandler();
  }

  inline void stop()
  {
    doPush(true, [](T &) {});
  }

  inline void push(const std::function< void (T &ioInternal) > &iFunction)
  {
    doPush(false, iFunction);
  }

private:
  T mInternal;
  std::deque< std::pair< bool, std::function< void (T &ioInternal) > > > mWorkQueue;
  std::mutex mQueueMutex;
  std::condition_variable mWorkSignal;

  void doPush(bool iTerminate, const std::function< void (T &ioInternal) > &iFunction)
  {
    {
      std::lock_guard< std::mutex > wLock(mQueueMutex);
      mWorkQueue.emplace_back(iTerminate, iFunction);
    }
    mWorkSignal.notify_one();
  }

  void workHandler()
  {
    while (true)
    {
      std::unique_lock< std::mutex > wLock(mQueueMutex);
      mWorkSignal.wait(wLock, [&]() { return !mWorkQueue.empty(); });
      auto &&wWorkItem = mWorkQueue.front();
      if (wWorkItem.first)
      {
        break;
      }
      wWorkItem.second(mInternal);
      mWorkQueue.pop_front();
    }
  }
};