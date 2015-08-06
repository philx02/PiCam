#pragma once

#include "CommonTypedefs.h"
#include "Operations.h"
#include "SrtBuilder.h"
#include "Subject.h"

class CameraAndLightControl : public Subject< CameraAndLightControl >
{
public:
  CameraAndLightControl(const char *iH264FilePath, const char *iSrtFilePath)
    : mCameraChild(openCameraChildProcess(iH264FilePath))
    , mDoorSwitch(false)
    , mRecording(false)
    , mLight(false)
    , mRecordingOverride(false)
    , mLightOverride(false)
    , mSrtBuilder(iSrtFilePath)
  {
    waitForCamera();
  }

  // VS2013 still does not support defaulted move constructors :(
  CameraAndLightControl(CameraAndLightControl &&iCameraAndLightControl)
    : Subject< CameraAndLightControl >(std::move(iCameraAndLightControl))
    , mCameraChild(std::move(iCameraAndLightControl.mCameraChild))
    , mDoorSwitch(std::move(iCameraAndLightControl.mDoorSwitch))
    , mRecording(std::move(iCameraAndLightControl.mRecording))
    , mLight(std::move(iCameraAndLightControl.mLight))
    , mRecordingOverride(std::move(iCameraAndLightControl.mRecordingOverride))
    , mLightOverride(std::move(iCameraAndLightControl.mLightOverride))
    , mSrtBuilder(std::move(iCameraAndLightControl.mSrtBuilder))
    , mStartRecord(std::move(iCameraAndLightControl.mStartRecord))
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
  CameraPid mCameraChild;
  bool mDoorSwitch;
  bool mRecording;
  bool mLight;
  bool mRecordingOverride;
  bool mLightOverride;
  SrtBuilder mSrtBuilder;
  HighResolutionClock::time_point mStartRecord;

  void startRecording()
  {
    if (!mRecording)
    {
      notifyCameraChildProcess(mCameraChild);
      mRecording = true;
      mStartRecord = sch::high_resolution_clock::now();
    }
  }

  void stopRecording()
  {
    if (mRecording)
    {
      notifyCameraChildProcess(mCameraChild);
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