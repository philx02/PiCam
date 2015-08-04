#pragma once

template< typename SubjectType >
class IObserver
{
public:
  virtual ~IObserver() {}
  virtual void update(const SubjectType &iSubject) = 0;
};
