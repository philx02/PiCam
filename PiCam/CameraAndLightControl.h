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
    , mRecording(false)
    , mSrtBuilder(iSrtFilePath)
  {
    waitForCamera();
  }

  // VS2013 still does not support defaulted move constructors :(
  CameraAndLightControl(CameraAndLightControl &&iCameraAndLightControl)
    : Subject< CameraAndLightControl >(std::move(iCameraAndLightControl))
    , mCameraChild(std::move(iCameraAndLightControl.mCameraChild))
    , mRecording(std::move(iCameraAndLightControl.mRecording))
    , mSrtBuilder(std::move(iCameraAndLightControl.mSrtBuilder))
    , mStartRecord(std::move(iCameraAndLightControl.mStartRecord))
  {
  }

  void startRecording()
  {
    if (!mRecording)
    {
      notifyCameraChildProcess(mCameraChild);
      mRecording = true;
      mStartRecord = sch::high_resolution_clock::now();
      notify(*this);
    }
  }

  void stopRecording()
  {
    if (mRecording)
    {
      notifyCameraChildProcess(mCameraChild);
      auto wRecordDuration = sch::high_resolution_clock::now() - mStartRecord;
      mSrtBuilder.append(mStartRecord, wRecordDuration);
      mRecording = false;
      notify(*this);
    }
  }

  void notifyOne(IObserver< CameraAndLightControl > &iObserver)
  {
    iObserver.update(*this);
  }

  bool recording() const
  {
    return mRecording;
  }

private:
  CameraPid mCameraChild;
  bool mRecording;
  SrtBuilder mSrtBuilder;
  HighResolutionClock::time_point mStartRecord;
};