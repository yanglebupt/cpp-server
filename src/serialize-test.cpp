#include "utils/serialization/data_stream.hpp"
#include <iostream>

struct App : public serializable
{
public:
  SERIALIZE(name, age);
  App(std::string name, float age) : name(name), age(age) {}

  void print()
  {
    std::cout << name << ":" << age << std::endl;
    for (size_t i = 0; i < ls.size(); i++)
    {
      std::cout << ls[i] << ",";
    }
    std::cout << std::endl;
  }

private:
  std::string name;
  float age;
  std::vector<std::string> ls{"Kile", "Jue", "Like"};
};

int main()
{
  data_stream ds(Endian::BE);

  std::vector<std::string> ls{"23.12", "43.23", "1.89"};
  std::map<std::string, int> ms{{"m", 23}, {"tom", 45}, {"jack", 65}};
  App app("App", 34.21);

  int &&a = 10;
  ds << a;
  ds << "Hello, world!";
  ds << ls;
  ds << ms;
  ds << app;
  ds << 10.32f;

  ds.save("./logs/buffer");
  ds.clear();
  ds.load("./logs/buffer");

  int t;
  std::string s;
  std::vector<std::string> lls;
  std::map<std::string, int> mms;
  App ap("Mike", 10.0f);
  float d;
  ds >> t >> s >> lls >> mms >> ap >> d;

  std::cout << t << "," << s << "," << d << std::endl;
  for (size_t i = 0; i < lls.size(); i++)
  {
    std::cout << lls[i] << ",";
  }

  std::cout << std::endl;
  for (auto it = mms.begin(); it != mms.end(); it++)
  {
    std::cout << it->first << "=" << it->second << ",";
  }
  std::cout << std::endl;

  ap.print();

  std::cout << ds << std::endl;

  return 0;
}