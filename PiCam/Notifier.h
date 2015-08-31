#pragma once

#include "sqlite/sqlite3.h"
#include "Statement.h"
#include "SendEmail.h"
#include "Operations.h"

#include <boost/noncopyable.hpp>
#include <boost/date_time.hpp>

#include <memory>
#include <stdexcept>

class Notifier : public boost::noncopyable
{
public:
  explicit Notifier(const char *iDbLocation)
    : mSqlite(nullptr)
  {
    auto wResult = sqlite3_open_v2(iDbLocation, &mSqlite, SQLITE_OPEN_READWRITE, nullptr);
    if (wResult != SQLITE_OK)
    {
      throw std::runtime_error(sqlite3_errstr(wResult));
    }
    Statement(mSqlite, "PRAGMA foreign_keys = ON").runOnce();
    mRetriveCoverageAlwaysOn.reset(new Statement(mSqlite, "SELECT value FROM parameters WHERE name='Coverage always on'"));
    mRetriveUrl.reset(new Statement(mSqlite, "SELECT value FROM parameters WHERE name='URL'"));
    mSetCoverageAlwaysOn.reset(new Statement(mSqlite, "UPDATE parameters SET value=?1 WHERE name='Coverage always on'"));
    mRetrieveCoverage.reset(new Statement(mSqlite, "SELECT weekday_begin, hour_begin, weekday_end, hour_end FROM coverage_intervals"));
    mSmsRecipients.reset(new Statement(mSqlite, "SELECT recipient_number, enabled FROM sms_recipients"));
    mEmailRecipients.reset(new Statement(mSqlite, "SELECT recipient_name, recipient_email, enabled FROM email_recipients"));
  }

  Notifier(Notifier &&iNotifier)
    : mSqlite(std::move(iNotifier.mSqlite))
    , mRetriveCoverageAlwaysOn(std::move(iNotifier.mRetriveCoverageAlwaysOn))
    , mRetriveUrl(std::move(iNotifier.mRetriveUrl))
    , mSetCoverageAlwaysOn(std::move(iNotifier.mSetCoverageAlwaysOn))
    , mRetrieveCoverage(std::move(iNotifier.mRetrieveCoverage))
    , mSmsRecipients(std::move(iNotifier.mSmsRecipients))
    , mEmailRecipients(std::move(iNotifier.mEmailRecipients))
  {
    iNotifier.mSqlite = nullptr;
  }

  ~Notifier()
  {
    while (mSqlite != nullptr && sqlite3_close(mSqlite) == SQLITE_BUSY)
    {
      std::this_thread::yield();
    }
  }

  void perform()
  {
    if (checkIfInCoverage())
    {
      sendSms();
      sendEmail();
    }
  }

  bool coverageAlwaysOn() const
  {
    bool wCoverageAlwaysOn = false;
    mRetriveCoverageAlwaysOn->clear();
    while (mRetriveCoverageAlwaysOn->runOnce() == SQLITE_ROW)
    {
      mRetriveCoverageAlwaysOn->evaluate([&](sqlite3_stmt *iStatement)
      {
        wCoverageAlwaysOn = std::strcmp(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), "1") == 0;
      });
    }
    return wCoverageAlwaysOn;
  }

  void coverageAlwaysOn(bool iValue)
  {
    mSetCoverageAlwaysOn->clear();
    mSetCoverageAlwaysOn->bind(1, std::string(iValue ? "1" : "0"));
    mSetCoverageAlwaysOn->runOnce();
  }

  struct CoverageInterval
  {
    CoverageInterval(boost::gregorian::greg_weekday iWeekdayBegin, boost::posix_time::time_duration::hour_type iHourBegin, boost::gregorian::greg_weekday iWeekdayEnd, boost::posix_time::time_duration::hour_type iHourEnd)
      : mWeekdayBegin(iWeekdayBegin)
      , mHourBegin(iHourBegin)
      , mWeekdayEnd(iWeekdayEnd)
      , mHourEnd(iHourEnd)
    {
    }
    boost::gregorian::greg_weekday mWeekdayBegin;
    boost::posix_time::time_duration::hour_type mHourBegin;
    boost::gregorian::greg_weekday mWeekdayEnd;
    boost::posix_time::time_duration::hour_type mHourEnd;
  };

  std::vector< CoverageInterval > coverageIntervals() const
  {
    std::vector< CoverageInterval > wReturn;
    mRetrieveCoverage->clear();
    while (mRetrieveCoverage->runOnce() == SQLITE_ROW)
    {
      mRetrieveCoverage->evaluate([&](sqlite3_stmt *iStatement)
      {
        wReturn.emplace_back(sqlite3_column_int(iStatement, 0), sqlite3_column_int(iStatement, 1), sqlite3_column_int(iStatement, 2), sqlite3_column_int(iStatement, 3));
      });
    }
    return wReturn;
  }

private:
  sqlite3 *mSqlite;
  std::unique_ptr< Statement > mRetriveCoverageAlwaysOn;
  std::unique_ptr< Statement > mRetriveUrl;
  std::unique_ptr< Statement > mSetCoverageAlwaysOn;
  std::unique_ptr< Statement > mRetrieveCoverage;
  std::unique_ptr< Statement > mSmsRecipients;
  std::unique_ptr< Statement > mEmailRecipients;

  std::string retrieveUrl()
  {
    std::string wUrl;
    mRetriveUrl->clear();
    while (mRetriveUrl->runOnce() == SQLITE_ROW)
    {
      mRetriveUrl->evaluate([&](sqlite3_stmt *iStatement)
      {
        wUrl.assign(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)));
      });
    }
    return wUrl;
  }

  void sendSms()
  {
    auto wUrl = retrieveUrl();
    mSmsRecipients->clear();
    while (mSmsRecipients->runOnce() == SQLITE_ROW)
    {
      mSmsRecipients->evaluate([&](sqlite3_stmt *iStatement)
      {
        if (sqlite3_column_int(iStatement, 1) == 1)
        {
          ::sendSms(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), wUrl);
        }
      });
    }
  }

  void sendEmail()
  {
    auto wUrl = retrieveUrl();
    mEmailRecipients->clear();
    while (mEmailRecipients->runOnce() == SQLITE_ROW)
    {
      mEmailRecipients->evaluate([&](sqlite3_stmt *iStatement)
      {
        if (sqlite3_column_int(iStatement, 2) == 1)
        {
          ::sendEmail(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 1)), wUrl);
        }
      });
    }
  }

  bool checkIfInCoverage()
  {
    auto wNow = boost::posix_time::second_clock::local_time();
    auto wHourOfTheWeek = wNow.date().day_of_week().as_number() * 24 + wNow.time_of_day().hours();
    return 
      checkCoverageForStatement(*mRetriveCoverageAlwaysOn, [](sqlite3_stmt *iStatement) -> bool
      {
        return std::strcmp(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), "1") == 0;
      })
      ||
      checkCoverageForStatement(*mRetrieveCoverage, [&](sqlite3_stmt *iStatement) -> bool
      {
        auto wCoverageInterval = CoverageInterval(boost::gregorian::greg_weekday(sqlite3_column_int(iStatement, 0)), sqlite3_column_int(iStatement, 1), boost::gregorian::greg_weekday(sqlite3_column_int(iStatement, 2)), sqlite3_column_int(iStatement, 3));
        auto wHourOfTheWeekCoverageBegin = wCoverageInterval.mWeekdayBegin.as_number() * 24 + wCoverageInterval.mHourBegin;
        auto wHourOfTheWeekCoverageEnd = wCoverageInterval.mWeekdayEnd.as_number() * 24 + wCoverageInterval.mHourEnd;
        return wHourOfTheWeek >= wHourOfTheWeekCoverageBegin && wHourOfTheWeek <= wHourOfTheWeekCoverageEnd;
      });
  }

  template< typename CoverageCheck >
  bool checkCoverageForStatement(Statement &ioStatement, const CoverageCheck &iCoverageCheck)
  {
    ioStatement.clear();
    while (ioStatement.runOnce() == SQLITE_ROW)
    {
      bool wCoverageIsOn = false;
      ioStatement.evaluate([&](sqlite3_stmt *iStatement)
      {
        wCoverageIsOn = iCoverageCheck(iStatement);
      });
      if (wCoverageIsOn)
      {
        return true;
      }
    }
    return false;
  }
};
