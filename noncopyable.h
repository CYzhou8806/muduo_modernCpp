#pragma once


class noncopyable
{
 public:
  noncopyable(const noncopyable&) = delete;
  void operator=(const noncopyable&) = delete;

 protected: // prevent users from directly creating instances
  noncopyable() = default;
  ~noncopyable() = default;
};

