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
  RemoteControl(DataActiveObject< CameraAndLightControl > &iCameraAndLightControl)
    : mCameraAndLightControl(iCameraAndLightControl)
  {
    std::mutex wSyncMutex;
    bool wObserverAttached = false;
    std::condition_variable wSyncSignal;
    mCameraAndLightControl.dataPush([&](CameraAndLightControl &iControl)
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
    mCameraAndLightControl.dataPush([&](CameraAndLightControl &iControl)
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
    mCameraAndLightControl.dataPush([&](CameraAndLightControl &iControl)
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
      mCameraAndLightControl.dataPush([this](CameraAndLightControl &iControl)
      {
        iControl.notifyOne(*this);
      });
      return;
    }
    std::vector< std::string > wSplit;
    boost::split(wSplit, iPayload, boost::is_any_of("|"));
    if (wSplit.size() == 2)
    {
      bool wValue = wSplit[1] == "1";
      if (wSplit[0] == "light_override")
      {
        mCameraAndLightControl.dataPush([wValue](CameraAndLightControl &iControl)
        {
          iControl.lightOverride(wValue);
        });
      }
      else if (wSplit[0] == "coverage_always_on")
      {
        mCameraAndLightControl.dataPush([wValue](CameraAndLightControl &iControl)
        {
          iControl.coverageAlwaysOn(wValue);
        });
      }
    }
  }

  void update(const CameraAndLightControl &iControl)
  {
    sendStatus(iControl);
  }

private:
  DataActiveObject< CameraAndLightControl > &mCameraAndLightControl;
  std::weak_ptr< ISender > mSender;

  void sendStatus(const CameraAndLightControl &iControl)
  {
    auto wSender = mSender.lock();
    if (wSender != nullptr)
    {
      std::string wMessage;
      wMessage += iControl.doorSwitch() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.light() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.lightOverride() ? "1" : "0";
      wMessage += "|";
      wMessage += iControl.coverageAlwaysOn() ? "1" : "0";
      auto wCoverageIntervals = iControl.coverageIntervals();
      for (auto &&wCoveragePeriod : wCoverageIntervals)
      {
        wMessage += "|";
        wMessage += std::to_string(wCoveragePeriod.mWeekdayBegin.as_number()) + "," + std::to_string(wCoveragePeriod.mHourBegin) + ";";
        wMessage += std::to_string(wCoveragePeriod.mWeekdayEnd.as_number()) + "," + std::to_string(wCoveragePeriod.mHourEnd);
      }
      wSender->send(wMessage);
    }
  }
};
