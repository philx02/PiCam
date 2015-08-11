#pragma once

#include "CommonTypedefs.h"
#include "Operations.h"
#include "SrtBuilder.h"
#include "Subject.h"
#include "Notifier.h"

#include <boost/noncopyable.hpp>

class CameraAndLightControl : public Subject< CameraAndLightControl >, public boost::noncopyable
{
public:
  CameraAndLightControl(const char *iCameraPid, const char *iSrtFilePath, const char *iNotifierDbLocation)
    : mCameraPid(std::strtol(iCameraPid, nullptr, 10))
    , mDoorSwitch(false)
    , mRecording(false)
    , mLight(false)
    , mRecordingOverride(false)
    , mLightOverride(false)
    , mSrtBuilder(iSrtFilePath)
    , mNotifier(iNotifierDbLocation)
  {
    waitForCamera();
  }

  // VS2013 still does not support defaulted move constructors :(
  CameraAndLightControl(CameraAndLightControl &&iCameraAndLightControl)
    : Subject< CameraAndLightControl >(std::move(iCameraAndLightControl))
    , mCameraPid(std::move(iCameraAndLightControl.mCameraPid))
    , mDoorSwitch(std::move(iCameraAndLightControl.mDoorSwitch))
    , mRecording(std::move(iCameraAndLightControl.mRecording))
    , mLight(std::move(iCameraAndLightControl.mLight))
    , mRecordingOverride(std::move(iCameraAndLightControl.mRecordingOverride))
    , mLightOverride(std::move(iCameraAndLightControl.mLightOverride))
    , mSrtBuilder(std::move(iCameraAndLightControl.mSrtBuilder))
    , mStartRecord(std::move(iCameraAndLightControl.mStartRecord))
    , mNotifier(std::move(iCameraAndLightControl.mNotifier))
  {
  }

  void doorSwitch(bool iValue)
  {
    mDoorSwitch = iValue;

    if (mDoorSwitch)
    {
      startRecording();
      lightOn();
    }
    else
    {
      if (!mRecordingOverride)
      {
        stopRecording();
      }
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

  bool recording() const
  {
    return mRecording;
  }

  bool light() const
  {
    return mLight;
  }

  bool recordingOverride() const
  {
    return mRecordingOverride;
  }

  bool lightOverride() const
  {
    return mLightOverride;
  }

  void recordingOverride(bool iOverride)
  {
    mRecordingOverride = iOverride;
    if (mRecordingOverride)
    {
      startRecording();
    }
    else if (!mDoorSwitch)
    {
      stopRecording();
    }
    notify(*this);
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

private:
  CameraPid mCameraPid;
  bool mDoorSwitch;
  bool mRecording;
  bool mLight;
  bool mRecordingOverride;
  bool mLightOverride;
  SrtBuilder mSrtBuilder;
  HighResolutionClock::time_point mStartRecord;
  Notifier mNotifier;

  void startRecording()
  {
    if (!mRecording)
    {
      notifyCameraChildProcess(mCameraPid);
      mRecording = true;
      mStartRecord = sch::high_resolution_clock::now();
    }
  }

  void stopRecording()
  {
    if (mRecording)
    {
      notifyCameraChildProcess(mCameraPid);
      mRecording = false;
      auto wRecordDuration = sch::high_resolution_clock::now() - mStartRecord;
      mSrtBuilder.append(mStartRecord, wRecordDuration);
    }
  }

  void lightOn()
  {
    mLight = true;
  }

  void lightOff()
  {
    mLight = false;
  }
};