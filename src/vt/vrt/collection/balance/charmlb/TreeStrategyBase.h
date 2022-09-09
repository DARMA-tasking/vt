#ifndef TREESTRATEGYBASE_H
#define TREESTRATEGYBASE_H

#include "vt/configs/types/types_type.h"

#include "vt/config.h"
#include "vt/vrt/collection/balance/charmlb/charmlb_types.h"
#include "vt/vrt/collection/balance/lb_common.h"

#include <algorithm>
#include <array>
#include <cassert>
#include <initializer_list>
#include <random>
#include <unordered_map>

#include <sstream>
#include <string>

using LoadFloatType = double;

namespace TreeStrategy
{
template <typename T, bool is_ptr = std::is_pointer<T>::value>
struct CmpLoadGreater
{
};

template <typename T>
struct CmpLoadGreater<T, true>
{
  inline bool operator()(const T a, const T b) const
  {
    return (a->getLoad() > b->getLoad());
  }
};

template <typename T>
struct CmpLoadGreater<T, false>
{
  inline bool operator()(const T& a, const T& b) const
  {
    return (a.getLoad() > b.getLoad());
  }
};

template <typename T, bool is_ptr = std::is_pointer<T>::value>
struct CmpLoadLess
{
};

template <typename T>
struct CmpLoadLess<T, true>
{
  inline bool operator()(const T a, const T b) const
  {
    return (a->getLoad() < b->getLoad());
  }
};

template <typename T>
struct CmpLoadLess<T, false>
{
  inline bool operator()(const T& a, const T& b) const
  {
    return (a.getLoad() < b.getLoad());
  }
};

template <typename T, bool is_ptr = std::is_pointer<T>::value>
struct CmpId
{
};

template <typename T>
struct CmpId<T, true>
{
  inline bool operator()(const T a, const T b) const { return (a->id < b->id); }
};

template <typename T>
struct CmpId<T, false>
{
  inline bool operator()(const T& a, const T& b) const { return (a.id < b.id); }
};

template <typename T>
T* ptr(T& obj)
{
  return &obj;
}  // turn reference into pointer

template <typename T>
T* ptr(T* obj)
{
  return obj;
}  // obj is already pointer, return it

// ---------------- Obj --------------------

struct obj_1_data
{
  LoadFloatType load;
};
template <int N>
struct obj_N_data
{
  LoadFloatType totalload;
  std::array<LoadFloatType, N> load = {};
};

template <int N, bool multi = (N > 1)>
class Obj : public std::conditional<multi, obj_N_data<N>, obj_1_data>::type
{
  using IDType = vt::vrt::collection::balance::ElementIDStruct;
 public:
  IDType id;
  //int oldPe;

  static constexpr auto dimension = N;

  inline Obj(IDType const& _id, const LoadFloatType _load, std::vector<LoadFloatType>&& vecLoad) //int _oldPe)
  {
    id = _id;
    //oldPe = _oldPe;
    this->totalload = _load;
    for (int i = 0; i < N; i++)
    {
      this->load[i] = vecLoad[i];
    }
  }

  inline LoadFloatType getLoad() const { return this->totalload; }
  inline LoadFloatType getLoad(int dim) const
  {
    return this->load[dim];
  }
  void setPosition(std::vector<float>& position) {}
  LoadFloatType operator[](size_t dim) const { return getLoad(dim); }
};

template <>
inline Obj<1>::Obj(IDType const& _id, const LoadFloatType _load, std::vector<LoadFloatType>&& vecLoad)// int _oldPe)
{
  id = _id;
  load = _load;
  //oldPe = _oldPe;
}

template <>
inline LoadFloatType Obj<1>::getLoad() const
{
  return load;
}

template <>
inline LoadFloatType Obj<1>::getLoad(int) const
{
  return getLoad();
}

template <int N>
class ObjPos : public Obj<N>
{
public:
  static constexpr bool isPosition = true;
  bool hasPosition = false;
  std::vector<float> position;

  inline void setPosition(std::vector<float>& pos)
  {
    assert(!position.empty());
    hasPosition = true;
    position = pos;
  }
};

// ------------------ Proc ------------------

/**
 * class Proc<int N, bool rateAware>
 * This just shows the interface. only the specializations (below) are defined
 * N: number of dimensions of vector load
 * rateAware: true if speed aware, false otherwise
 */
template <int N, bool rateAware, bool multi = (N > 1)>
class Proc
{
 public:
  LoadFloatType getLoad() const;         // returns current load of processor
  LoadFloatType getLoad(int dim) const;  // returns current load of processor
  LoadFloatType operator[](size_t dim) const;
  void assign(const Obj<N>* o);  // add object loads to this processor's loads
  void assign(const Obj<N>& o);  // add object loads to this processor's loads

  void resetLoad();              // sets processor loads to background loads
  bool operator==(const Proc& element) const;
};

template <int N>
struct proc_N_data
{
  std::array<LoadFloatType, N> load = {};
  std::array<LoadFloatType, N> bgload = {};
  LoadFloatType totalload = 0;
};
struct proc_1_data
{
  LoadFloatType load = 0;
  LoadFloatType bgload = 0;
};

// --------- Proc rateAware=false specializations ---------

template <int N, bool multi>
class Proc<N, false, multi>
    : public std::conditional<multi, proc_N_data<N>, proc_1_data>::type {
  using NodeType = vt::NodeType;
  using ObjIDType = vt::vrt::collection::balance::ElementIDStruct;

public:
  NodeType id = vt::uninitialized_destination;
  static constexpr auto dimension = N;

  inline Proc(NodeType const& _id, LoadFloatType _bgload, LoadFloatType* _speed)
  {
    id = _id;
    // TODO: implement vector bgload
    std::fill_n(this->bgload.begin(), N, _bgload / N);
  }

  inline LoadFloatType getLoad() const { return this->totalload; }

  inline LoadFloatType getLoad(int dim) const
  {
    assert(dim < dimension);
    return this->load[dim];
  }

  LoadFloatType operator[](size_t dim) const { return getLoad(dim); }

  inline void assign(const Obj<N>* o)
  {
    for (int i = 0; i < N; i++)
    {
      this->load[i] += o->load[i];
      this->totalload += o->load[i];
    }
  }
  inline void assign(const Obj<N>& o) { assign(&o); }

  inline void resetLoad()
  {
    this->totalload = 0;
    for (int i = 0; i < N; i++)
    {
      this->load[i] = this->bgload[i];
      this->totalload += this->bgload[i];
    }
  }

  bool operator==(const Proc& element) const { return id == element.id; };
};

template <>
Proc<1, false>::Proc(NodeType const& _id, LoadFloatType _bgload, LoadFloatType* _speed)
{
  id = _id;
  this->bgload = _bgload;
}
template <>
LoadFloatType Proc<1, false>::getLoad() const
{
  return this->load;
}
template <>
LoadFloatType Proc<1, false>::getLoad(int) const
{
  return getLoad();
}
template <>
void Proc<1, false>::assign(const Obj<1>* o)
{
  this->load += o->load;
}

template <>
void Proc<1, false>::resetLoad()
{
  this->load = this->bgload;
}

// --------- Proc rateAware=true specializations ---------

// TODO further specialize for N=1 by having speed not be an array?

template <int N, bool multi>
class Proc<N, true, multi>
    : public std::conditional<multi, proc_N_data<N>, proc_1_data>::type
{
  using NodeType = vt::NodeType;
  using ObjIDType = vt::vrt::collection::balance::ElementIDStruct;

public:
  NodeType id = vt::uninitialized_destination;
  static constexpr auto dimension = N;
  std::array<LoadFloatType, N> speed;

  inline Proc(NodeType const& _id, LoadFloatType _bgload, LoadFloatType* _speed)
  {
    id = _id;
    std::fill_n(this->bgload.begin(), N, _bgload / N);
    std::copy_n(_speed, N, this->speed.begin());
  }

  inline LoadFloatType getLoad() const { return this->totalload; }

  inline LoadFloatType getLoad(int dim) const
  {
    assert(dim < dimension);
    return this->load[dim];
  }

  LoadFloatType operator[](size_t dim) const { return getLoad(dim); }

  inline void assign(const Obj<N>* o)
  {
    for (int i = 0; i < N; i++)
    {
      this->load[i] += (o->load[i] / speed[i]);
      this->totalload += (o->load[i] / speed[i]);
    }
  }
  inline void assign(const Obj<N>& o) { assign(&o); }

  inline void resetLoad()
  {
    this->totalload = 0;
    for (int i = 0; i < N; i++)
    {
      this->load[i] = this->bgload[i];
      this->totalload += this->bgload[i];
    }
  }

  bool operator==(const Proc& element) const { return id == element.id; };
};

template <>
Proc<1, true>::Proc(NodeType const& _id, LoadFloatType _bgload, LoadFloatType* _speed)
{
  id = _id;
  this->bgload = _bgload;
  speed[0] = _speed[0];
}
template <>
LoadFloatType Proc<1, true>::getLoad() const
{
  return this->load;
}
template <>
LoadFloatType Proc<1, true>::getLoad(int) const
{
  return getLoad();
}
template <>
LoadFloatType Proc<1, true>::operator[](size_t) const
{
  return getLoad();
}
template <>
void Proc<1, true>::assign(const Obj<1>* o)
{
  this->load += (o->load / speed[0]);
}
template <>
void Proc<1, true>::resetLoad()
{
  this->load = this->bgload;
}

template <typename O, typename P>
class Solution {
private:
  std::unordered_map<vt::NodeType, size_t> id2index;

public:
  std::vector<vt::vrt::collection::lb::CharmDecision> decisions;

  Solution(const std::vector<P> &procs) {
    for (const auto &p : procs) {
      id2index[p.id] = decisions.size();
      decisions.emplace_back(
          p.id,
          std::initializer_list<vt::vrt::collection::lb::CharmLBTypes::ObjIDType>(
              {}));
    }
  }
  void assign(const O &obj, P &proc) {
    proc.assign(obj);
    decisions[id2index[proc.id]].objs_.emplace_back(obj.id);
  }
};

// ---------------- Strategy --------------------

template <typename O, typename P, typename S>
class Strategy
{
 public:
  virtual void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution,
                     bool objsSorted = false) = 0;
  virtual ~Strategy() {}
};

template <typename O, typename P, typename S>
class Random : public Strategy<O, P, S>
{
 public:
  Random()
  {
    std::random_device rd;
    rng = std::mt19937(rd());
  }
  void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution, bool objsSorted)
  {
    std::uniform_int_distribution<int> uni(0, procs.size() - 1);
    for (const auto& o : objs) solution.assign(o, procs[uni(rng)]);
  }

 private:
  std::mt19937 rng;
};

template <typename O, typename P, typename S>
class Dummy : public Strategy<O, P, S>
{
 public:
  void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution, bool objsSorted) {
    for (const auto& o : objs)
      solution.assign(o, procs[ptr(o)->oldPe]);
  }  // do nothing
};

template <typename It>
std::string join(It begin, It end, const std::string& separator = ", ") {
  std::ostringstream output;

  if (begin != end) {
    output << *begin++;
  }

  while (begin != end) {
    output << separator;
    output << *begin++;
  }

  return output.str();
}
}  // namespace TreeStrategy

#endif /* TREESTRATEGYBASE_H */
