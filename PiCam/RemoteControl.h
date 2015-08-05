#pragma once

#include "IObserver.h"
#include "CameraAndLightControl.h"
#include "ActiveObject.h"
#include "TcpServer/ISender.h"

#include <mutex>
#include <condition_variable>

class RemoteControl : public IObserver< CameraAndLightControl >
{
public:
  RemoteControl(ActiveObject< CameraAndLightControl > &iCameraAndLightControl)
    : mCameraAndLightControl(iCameraAndLightControl)
  {
    std::mutex wSyncMutex;
    bool wObserverAttached = false;
    std::condition_variable wSyncSignal;
    mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
    {
      iControl.attach(this);
      {
        std::lock_guard< decltype(wSyncMutex) > wLock(wSyncMutex);
        wObserverAttached = true;
      }
      wSyncSignal.notify_one();
    });
    std::unique_lock< decltype(wSyncMutex) > wLock(wSyncMutex);
    wSyncSignal.wait(wLock, [&]() { return wObserverAttached; });
  }

  RemoteControl(const RemoteControl &iRemoteControl)
    : mCameraAndLightControl(iRemoteControl.mCameraAndLightControl)
  {
    std::mutex wSyncMutex;
    bool wObserverAttached = false;
    std::condition_variable wSyncSignal;
    mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
    {
      iControl.attach(this);
      {
        std::lock_guard< decltype(wSyncMutex) > wLock(wSyncMutex);
        wObserverAttached = true;
      }
      wSyncSignal.notify_one();
    });
    std::unique_lock< decltype(wSyncMutex) > wLock(wSyncMutex);
    wSyncSignal.wait(wLock, [&]() { return wObserverAttached; });
  }

  ~RemoteControl()
  {
    std::mutex wSyncMutex;
    bool wObserverDetached = false;
    std::condition_variable wSyncSignal;
    mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
    {
      iControl.detach(this);
      {
        std::lock_guard< decltype(wSyncMutex) > wLock(wSyncMutex);
        wObserverDetached = true;
      }
      wSyncSignal.notify_one();
    });
    std::unique_lock< decltype(wSyncMutex) > wLock(wSyncMutex);
    wSyncSignal.wait(wLock, [&]() { return wObserverDetached; });
  }

  void setSender(const std::shared_ptr< ISender > &iSender)
  {
    mSender = iSender;
  }

  void operator()(const std::string &iPayload)
  {
    if (iPayload == "get_status")
    {
      mCameraAndLightControl.push([&](CameraAndLightControl &iControl)
      {
        iControl.notifyOne(*this);
      });
    }
  }

  void update(const CameraAndLightControl &iControl)
  {
    sendStatus(iControl);
  }

private:
  ActiveObject< CameraAndLightControl > &mCameraAndLightControl;
  std::weak_ptr< ISender > mSender;

  void sendStatus(const CameraAndLightControl &iControl)
  {
    auto wSender = mSender.lock();
    if (wSender != nullptr)
    {
      wSender->send(std::string("recording|") + (iControl.recording() ? "1" : "0"));
    }
  }
};
