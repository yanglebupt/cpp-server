#pragma once

#include <string>
#include <map>

class Prop
{
public:
  virtual ~Prop() {};
};

template <typename T>
class Property : public Prop
{
private:
  T data;

public:
  virtual ~Property() {};

  Property(T &&d) : data(std::move(d)) {};
  Property(const T &d) : data(d) {};

  void SetValue(const T &d) { data = d; }
  void SetValue(T &&d) { data = std::move(d); }

  const T &GetValue() const { return data; }
};

class Properties
{
private:
  std::map<std::string, Prop *> props;

public:
  ~Properties()
  {
    for (auto &prop : props)
    {
      delete prop.second;
    }
    props.clear();
  };

  template <typename T>
  void Add(const std::string &name, T data) // 这里不能用 T&&，会有引用折叠，导致 T 是一个引用，而不是单类型
  {
    if (props.find(name) != props.end())
    {
      Property<T> *target = dynamic_cast<Property<T> *>(props[name]);
      target->SetValue(std::move(data));
    }
    else
      props[name] = new Property<T>(std::move(data));
  }

  template <typename T>
  const T &Get(const std::string &name)
  {
    if (props.find(name) != props.end())
    {
      Property<T> *p = dynamic_cast<Property<T> *>(props[name]);
      return p->GetValue();
    }
    else
      throw std::runtime_error("property [" + name + "] not found");
  }

  bool Has(const std::string &name)
  {
    return props.find(name) != props.end();
  }
};