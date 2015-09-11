#pragma once

#include <string>

class ISender
{
public:
  enum class MessageType
  {
    TEXT,
    BINARY
  };

  virtual void send(const std::string &iMessage, MessageType iMessageType = MessageType::TEXT) = 0;
  virtual size_t getId() const = 0;
};
