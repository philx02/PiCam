#pragma once

#include "Operations.h"
#include "Subject.h"
#include "Notifier.h"

#include <boost/noncopyable.hpp>

#include <memory>
#include <fstream>

void outputDate(std::ostream &iOut)
{
  auto wNow = std::time(nullptr);
  char wDateString[100];
  if (std::strftime(wDateString, sizeof(wDateString), "%Y-%m-%d %H:%M:%S", std::localtime(&wNow)) > 0)
  {
    iOut << "[" << wDateString << "] ";
  }
}

class CameraAndLightControl : public Subject< CameraAndLightControl >, public boost::noncopyable
{
public:
  CameraAndLightControl(const char *iNotifierDbLocation, const char *iGpioLightSwitch)
    : mGpioLightSwitch(new std::ofstream(iGpioLightSwitch))
    , mDoorSwitch(false)
    , mLight(false)
    , mLightOverride(false)
    , mNotifier(iNotifierDbLocation)
  {
  }

  // VS2013 still does not support defaulted move constructors :(
  CameraAndLightControl(CameraAndLightControl &&iCameraAndLightControl)
    : Subject< CameraAndLightControl >(std::move(iCameraAndLightControl))
    , mGpioLightSwitch(std::move(iCameraAndLightControl.mGpioLightSwitch))
    , mDoorSwitch(std::move(iCameraAndLightControl.mDoorSwitch))
    , mLight(std::move(iCameraAndLightControl.mLight))
    , mLightOverride(std::move(iCameraAndLightControl.mLightOverride))
    , mNotifier(std::move(iCameraAndLightControl.mNotifier))
  {
  }

  ~CameraAndLightControl()
  {
    if (mGpioLightSwitch != nullptr)
    {
      lightOff();
    }
  }

  void doorSwitch(bool iValue)
  {
    mDoorSwitch = iValue;
    outputDate(std::cout);
    std::cout << "Door " << ([&]() { return mDoorSwitch ? "OPENED" : "CLOSED"; })() << std::endl;

    if (mDoorSwitch)
    {
      lightOn();
      mNotifier.perform();
    }
    else
    {
      if (!mLightOverride)
      {
        lightOff();
      }
    }

    notify(*this);
  }

  void notifyOne(IObserver< CameraAndLightControl > &iObserver)
  {
    iObserver.update(*this);
  }

  bool doorSwitch() const
  {
    return mDoorSwitch;
  }

  bool light() const
  {
    return mLight;
  }

  bool lightOverride() const
  {
    return mLightOverride;
  }

  bool coverageAlwaysOn() const
  {
    return mNotifier.coverageAlwaysOn();
  }
  
  void lightOverride(bool iOverride)
  {
    mLightOverride = iOverride;
    if (mLightOverride)
    {
      lightOn();
    }
    else if (!mDoorSwitch)
    {
      lightOff();
    }
    notify(*this);
  }

  void coverageAlwaysOn(bool iValue)
  {
    mNotifier.coverageAlwaysOn(iValue);
    notify(*this);
  }

  std::vector< Notifier::CoverageInterval > coverageIntervals() const
  {
    return mNotifier.coverageIntervals();
  }

private:
  std::unique_ptr< std::ofstream > mGpioLightSwitch;
  bool mDoorSwitch;
  bool mLight;
  bool mLightOverride;
  Notifier mNotifier;

  void lightOn()
  {
    if (!mLight)
    {
      mLight = true;
      outputDate(std::cout);
      std::cout << "Light ON" << std::endl;
      *mGpioLightSwitch << "1";
      mGpioLightSwitch->flush();
    }
  }

  void lightOff()
  {
    if (mLight)
    {
      mLight = false;
      outputDate(std::cout);
      std::cout << "Light OFF" << std::endl;
      *mGpioLightSwitch << "0";
      mGpioLightSwitch->flush();
    }
  }
};