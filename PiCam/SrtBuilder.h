#pragma once

#include "CommonTypedefs.h"

#include <chrono>
#include <fstream>
#include <sstream>
#include <iomanip>
#include <memory>

class SrtBuilder
{
public:
  SrtBuilder(const char *iSrtLocation)
    : mSrtMemory(new std::ofstream(iSrtLocation))
    , mSrt(*mSrtMemory)
    , mEntries(0)
    , mMovieDuration(0)
  {
    mSrt << std::setfill('0');
  }
  
  // VS2013 still does not support defaulted move constructors :(
  SrtBuilder(SrtBuilder &&iSrtBuilder)
    : mSrtMemory(std::move(iSrtBuilder.mSrtMemory))
    , mSrt(*mSrtMemory)
    , mEntries(std::move(iSrtBuilder.mEntries))
    , mMovieDuration(std::move(iSrtBuilder.mMovieDuration))
  {
  }

  void append(const HighResolutionClock::time_point &iStartRecord, const HighResolutionClockDuration &iRecordDuration)
  {
    //mSrt << sch::duration_cast< sch::milliseconds >(iRecordDuration).count() << std::endl;
    if (sch::time_point_cast< sch::seconds >(iStartRecord) == sch::time_point_cast< sch::seconds >(iStartRecord + iRecordDuration))
    {
      if (iRecordDuration < sch::milliseconds(1))
      {
        mMovieDuration += iRecordDuration;
      }
      else
      {
        mMovieDuration += writeEntry(mMovieDuration, iStartRecord, iRecordDuration);
      }
      return;
    }

    auto wFinalMovieDuration = mMovieDuration + iRecordDuration;
    auto wMovieDurationIter = mMovieDuration;
        
    auto wTruncatedTime = sch::time_point_cast< sch::seconds >(iStartRecord);
    auto wFirstFractionOfSecond = sch::seconds(1) - (iStartRecord - wTruncatedTime);
    wMovieDurationIter += writeEntry(wMovieDurationIter, iStartRecord, wFirstFractionOfSecond);

    auto wWallClockIter = iStartRecord + sch::seconds(1);
    while (wMovieDurationIter + sch::seconds(1) <= wFinalMovieDuration)
    {
      wMovieDurationIter += writeEntry(wMovieDurationIter, wWallClockIter, sch::seconds(1));
      wWallClockIter += sch::seconds(1);
    }

    if (wMovieDurationIter != wFinalMovieDuration)
    {
      wMovieDurationIter += writeEntry(wMovieDurationIter, wWallClockIter, wFinalMovieDuration - wMovieDurationIter);
    }

    mMovieDuration = wFinalMovieDuration;
  }

private:
  std::unique_ptr< std::ofstream > mSrtMemory;
  std::ofstream &mSrt;
  size_t mEntries;
  HighResolutionClockDuration mMovieDuration;

  std::string formatTimeForEntry(const HighResolutionClockDuration &iTimeSinceBegining)
  {
    auto wHours = sch::duration_cast< sch::hours >(iTimeSinceBegining);
    auto wMinutes = sch::duration_cast< sch::minutes >(iTimeSinceBegining - wHours);
    auto wSeconds = sch::duration_cast< sch::seconds >(iTimeSinceBegining - wHours - wMinutes);
    auto wMilliseconds = sch::duration_cast< sch::milliseconds >(iTimeSinceBegining - wHours - wMinutes - wSeconds);
    std::ostringstream wReturn;
    wReturn << std::setfill('0');
    wReturn << std::setw(2) << wHours.count() << ":";
    wReturn << std::setw(2) << wMinutes.count() << ":";
    wReturn << std::setw(2) << wSeconds.count() << ",";
    wReturn << std::setw(3) << wMilliseconds.count();
    return wReturn.str();
  }

  const HighResolutionClockDuration & writeEntry(const HighResolutionClockDuration &iMovieDuration, const HighResolutionClock::time_point &iTime, const HighResolutionClockDuration &iDuration)
  {
    mSrt << ++mEntries << '\n';
    mSrt << formatTimeForEntry(iMovieDuration);
    mSrt << " --> ";
    mSrt << formatTimeForEntry(iMovieDuration + iDuration) << '\n';
    {
      auto wTimeT = sch::system_clock::to_time_t(iTime);
      // std::put_time is not implemented in GCC 4.8 :(
      //mSrt << std::put_time(std::localtime(&wTimeT), "%Y-%m-%d %X") << '\n';
      auto wTime = std::localtime(&wTimeT);
      mSrt << (wTime->tm_year + 1900) << "-" << std::setw(2) << (wTime->tm_mon + 1) << "-" << std::setw(2) << wTime->tm_mday << " " << std::setw(2) << wTime->tm_hour << ":" << std::setw(2) << wTime->tm_min << ":" << std::setw(2) << wTime->tm_sec << '\n';
    }
    mSrt << std::endl;
    return iDuration;
  }
};
