#pragma once

#include "sqlite/sqlite3.h"
#include "Statement.h"

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
    mRetrieve24_7IsOn.reset(new Statement(mSqlite, "SELECT value FROM parameters WHERE name='24/7 Coverage'"));
    mRetrieveCoverage.reset(new Statement(mSqlite, "SELECT weekday_start, time_start, weekday_end, time_end FROM coverage_intervals"));
    mSmsRecipients.reset(new Statement(mSqlite, "SELECT recipient FROM sms_recipients"));
    mEmailRecipients.reset(new Statement(mSqlite, "SELECT recipient FROM email_recipients"));
  }

  Notifier(Notifier &&iNotifier)
    : mSqlite(std::move(iNotifier.mSqlite))
    , mRetrieve24_7IsOn(std::move(iNotifier.mRetrieve24_7IsOn))
    , mRetrieveCoverage(std::move(iNotifier.mRetrieveCoverage))
  {
  }

  ~Notifier()
  {
    while (sqlite3_close(mSqlite) == SQLITE_BUSY)
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

private:
  sqlite3 *mSqlite;
  std::unique_ptr< Statement > mRetrieve24_7IsOn;
  std::unique_ptr< Statement > mRetrieveCoverage;
  std::unique_ptr< Statement > mSmsRecipients;
  std::unique_ptr< Statement > mEmailRecipients;

  void sendSms()
  {
  }

  void sendEmail()
  {
  }

  bool checkIfInCoverage()
  {
    return 
      checkCoverageForStatement(*mRetrieve24_7IsOn, [](sqlite3_stmt *iStatement) -> bool
      {
        return sqlite3_column_int(iStatement, 0) == 1;
      })
      ||
      checkCoverageForStatement(*mRetrieveCoverage, [](sqlite3_stmt *iStatement) -> bool
      {
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
  }
};