#pragma once

#include <cstdarg>
#include <cstdint>

template <typename... Args>
constexpr uint32_t get_size_of(const Args &...args)
{
  return (sizeof(args) + ...);
}

#define SERIALIZE(...)                                       \
  uint32_t __size = get_size_of(__VA_ARGS__);                \
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