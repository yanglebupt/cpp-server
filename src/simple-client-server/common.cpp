#pragma once
#include <type_traits>
#include <cstdint>

template <typename T>
typename std::underlying_type<T>::type PrintEnum(T const value)
{
  return static_cast<typename std::underlying_type<T>::type>(value);
}

enum class CustomMsgType : uint32_t
{
  ServerPing,
  MessageAll,
  ServerMessage,
};