#pragma once

#include "sqlite/sqlite3.h"
#include "Statement.h"
#include "SendEmail.h"

#include <boost/noncopyable.hpp>

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
    mSetCoverageAlwaysOn.reset(new Statement(mSqlite, "UPDATE parameters SET value=?1 WHERE name='Coverage always on'"));
    mRetrieveCoverage.reset(new Statement(mSqlite, "SELECT weekday_begin, hour_begin, weekday_end, hour_end FROM coverage_intervals"));
    mSmsRecipients.reset(new Statement(mSqlite, "SELECT recipient_number, enabled FROM sms_recipients"));
    mEmailRecipients.reset(new Statement(mSqlite, "SELECT recipient_name, recipient_email, enabled FROM email_recipients"));
  }

  Notifier(Notifier &&iNotifier)
    : mSqlite(std::move(iNotifier.mSqlite))
    , mRetriveCoverageAlwaysOn(std::move(iNotifier.mRetriveCoverageAlwaysOn))
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
    mRetriveCoverageAlwaysOn->clear();
    bool wCoverageAlwaysOn = false;
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
    CoverageInterval(uint8_t iWeekdayBegin, uint8_t iHourBegin, uint8_t iWeekdayEnd, uint8_t iHourEnd)
      : mWeekdayBegin(iWeekdayBegin)
      , mHourBegin(iHourBegin)
      , mWeekdayEnd(iWeekdayEnd)
      , mHourEnd(iHourEnd)
    {
    }
    uint8_t mWeekdayBegin;
    uint8_t mHourBegin;
    uint8_t mWeekdayEnd;
    uint8_t mHourEnd;
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
  std::unique_ptr< Statement > mSetCoverageAlwaysOn;
  std::unique_ptr< Statement > mRetrieveCoverage;
  std::unique_ptr< Statement > mSmsRecipients;
  std::unique_ptr< Statement > mEmailRecipients;

  void sendSms()
  {
    mSmsRecipients->clear();
    while (mSmsRecipients->runOnce() == SQLITE_ROW)
    {
      mSmsRecipients->evaluate([](sqlite3_stmt *iStatement)
      {
        if (sqlite3_column_int(iStatement, 1) == 1)
        {
          std::string wCommand = "curl \"https://voip.ms/api/v1/rest.php?api_username=pcayouette@spoluck.ca&api_password=0TH7zRXKINXj7Exz8S0c&method=sendSMS&did=4503141161&dst=";
          wCommand += reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0));
          wCommand += "&message=Door%20opened%20during%20coverage%20time.\" &";
          system(wCommand.c_str());
        }
      });
    }
  }

  void sendEmail()
  {
    mEmailRecipients->clear();
    while (mEmailRecipients->runOnce() == SQLITE_ROW)
    {
      mEmailRecipients->evaluate([](sqlite3_stmt *iStatement)
      {
        if (sqlite3_column_int(iStatement, 2) == 1)
        {
          ::sendEmail(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 1)));
        }
      });
    }
  }

  bool checkIfInCoverage()
  {
    return 
      checkCoverageForStatement(*mRetriveCoverageAlwaysOn, [](sqlite3_stmt *iStatement) -> bool
      {
        return std::strcmp(reinterpret_cast< const char * >(sqlite3_column_text(iStatement, 0)), "1") == 0;
      })
      ||
      checkCoverageForStatement(*mRetrieveCoverage, [](sqlite3_stmt *iStatement) -> bool
      {
        auto wCoverageInterval = CoverageInterval(sqlite3_column_int(iStatement, 0), sqlite3_column_int(iStatement, 1), sqlite3_column_int(iStatement, 2), sqlite3_column_int(iStatement, 3));
        return sqlite3_column_int(iStatement, 0) == 1;
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
