 // // 检查 middleware_map 是否是类T的成员
  // template <typename T>
  // struct has_middleware_map
  // {
  //   template <typename U>
  //   static void check(decltype(&U::middleware_map));

  //   template <typename U>
  //   static int check(...);

  //   enum
  //   {
  //     value = std::is_void<decltype(check<T>(0))>::value
  //   };
  // };

  // template <typename T>
  // bool is_middleware_collection(T var)
  // {
  //   return has_middleware_map<decltype(var)>::value && std::is_invocable<decltype(var), Request &, Response &, NextFunction>::value;
  // };