#pragma once

#include <cstdarg>

#define SERIALIZE(...)                                       \
  virtual void serialize(data_stream &stream) const override \
  {                                                          \
    stream.write_args(__VA_ARGS__);                          \
  };                                                         \
  virtual void deserialize(data_stream &stream) override     \
  {                                                          \
    stream.read_args(__VA_ARGS__);                           \
  };

class data_stream;
class serializable
{
public:
  virtual ~serializable() {}
  virtual void serialize(data_stream &stream) const {};
  virtual void deserialize(data_stream &stream) {};
};