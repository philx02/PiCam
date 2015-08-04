#pragma once

#include "IObserver.h"

#include <vector>
#include <algorithm>

template< typename SubjectType >
class Subject
{
public:
  inline void attach(IObserver< SubjectType > *iObserver)
  {
    mObservers.emplace_back(iObserver);
  }
  inline void detach(IObserver< SubjectType > *iObserver)
  {
    std::remove(mObservers.begin(), mObservers.end(), iObserver);
  }

protected:
  inline void notify(const SubjectType &iThis)
  {
    for (auto && iObserver : mObservers)
    {
      iObserver->update(iThis);
    }
  }

private:
  std::vector< IObserver< SubjectType > * > mObservers;
};
