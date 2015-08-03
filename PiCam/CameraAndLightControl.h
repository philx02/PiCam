#pragma once

#include "CommonTypedefs.h"
#include "Operations.h"
#include "SrtBuilder.h"

class CameraAndLightControl
{
public:
  CameraAndLightControl(const char *iH264FilePath, const char *iSrtFilePath)
    : mCameraChild(openCameraChildProcess(iH264FilePath))
    , mSrtBuilder(iSrtFilePath)
  {
  }

  // VS2013 still does not support defaulted move constructors :(
  CameraAndLightControl(CameraAndLightControl &&iCameraAndLightControl)
    : mCameraChild(std::move(iCameraAndLightControl.mCameraChild))
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
    }
  }

private:
  CameraPid mCameraChild;
  bool mRecording;
  SrtBuilder mSrtBuilder;
  HighResolutionClock::time_point mStartRecord;
};