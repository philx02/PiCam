#pragma once

#include "IObserver.h"
#include "CameraAndLightControl.h"
#include "ActiveObject.h"
#include "TcpServer/ISender.h"

#include <boost/algorithm/string.hpp>

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
      mCameraAndLightControl.push([this](CameraAndLightControl &iControl)
      {
        iControl.notifyOne(*this);
      });
      return;
    }
    std::vector< std::string > wSplit;
    boost::split(wSplit, iPayload, boost::is_any_of("|"));
    if (wSplit.size() == 2)
    {
      bool wOverride = wSplit[1] == "1";
      if (wSplit[0] == "recording_override")
      {
        mCameraAndLightControl.push([wOverride](CameraAndLightControl &iControl)
        {
          iControl.recordingOverride(wOverride);
        });
      }
      else if (wSplit[0] == "light_override")
      {
        mCameraAndLightControl.push([wOverride](CameraAndLightControl &iControl)
        {
          iControl.lightOverride(wOverride);
        });
      }
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
      std::string wMessage;
      wMessage += iControl.doorSwitch() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.recording() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.light() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.recordingOverride() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.lightOverride() ? "1" : "0";
      wSender->send(wMessage);
    }
  }
};
