#ifndef GREEDY_H_
#define GREEDY_H_

#include "TreeStrategyBase.h"
//#include "pheap.h"

#include <algorithm>
#include <cmath>
#include <iostream>
#include <queue>
#include <random>
#include <vector>

namespace TreeStrategy
{
template <typename O, typename P, typename S>
class Greedy : public Strategy<O, P, S>
{
public:
  void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution,
             bool objsSorted = false)
  {
    if (!objsSorted) std::sort(objs.begin(), objs.end(), CmpLoadGreater<O>());
    std::priority_queue<P, std::vector<P>, CmpLoadGreater<P>> procHeap(
        CmpLoadGreater<P>(), procs);

    for (const auto& o : objs)
    {
      P p = procHeap.top();
      procHeap.pop();
      solution.assign(o, p);  // update solution (assumes solution updates processor load)
      procHeap.push(p);
    }
  }
};

template <typename P, typename S>
class Greedy<Obj<1>, P, S> : public Strategy<Obj<1>, P, S>
{
public:
  void solve(std::vector<Obj<1>>& objs, std::vector<P>& procs, S& solution,
             bool objsSorted)
  {
    if (!objsSorted) std::sort(objs.begin(), objs.end(), CmpLoadGreater<Obj<1>>());
    std::priority_queue<P, std::vector<P>, CmpLoadGreater<P>> procHeap(
        CmpLoadGreater<P>(), procs);

    for (const auto& o : objs)
    {
      P p = procHeap.top();
      procHeap.pop();
      solution.assign(o, p);  // update solution (assumes solution updates processor load)
      procHeap.push(p);
    }
  }
};

}

/* namespace TreeStrategy */
/* { */
/* template <typename O, typename P, typename S> */
/* class Greedy : public Strategy<O, P, S> */
/* { */
/*  public: */
/*   void solve(std::vector<O>& objs, std::vector<P>& procs, S& solution, bool objsSorted) */
/*   { */
/*     // Sorts by maxload in vector */
/*     if (!objsSorted) std::sort(objs.begin(), objs.end(), CmpLoadGreater<O>()); */

/*     std::vector<ProcHeap<P>> heaps; */
/*     for (int i = 0; i < O::dimension; i++) */
/*     { */
/*       heaps.push_back(ProcHeap<P>(procs, i)); */
/*     } */

/*     std::array<float, O::dimension> averageLoad; */

/*     for (const auto& o : objs) */
/*     { */
/*       for (int i = 0; i < O::dimension; i++) */
/*       { */
/*         averageLoad[i] += o.load[i]; */
/*       } */
/*     } */

/*     for (int i = 0; i < O::dimension; i++) */
/*     { */
/*       averageLoad[i] /= objs.size(); */
/*     } */

/*     for (const auto& o : objs) */
/*     { */
/*       int maxdimension = 0; */
/*       float maxfactor = 0; */
/*       for (int i = 0; i < O::dimension; i++) */
/*       { */
/*         if (o.load[i] / averageLoad[i] > maxfactor) */
/*         { */
/*           maxfactor = o.load[i] / averageLoad[i]; */
/*           maxdimension = i; */
/*         } */
/*       } */
/*       P p = heaps[maxdimension].top(); */
/*       solution.assign(o, p);  // update solution (assumes solution updates processor load) */
/*       for (int i = 0; i < O::dimension; i++) */
/*       { */
/*         heaps[i].remove(p); */
/*         heaps[i].push(p); */
/*       } */
/*     } */
/*   } */
/* }; */

/* template <typename P, typename S> */
/* class Greedy<Obj<1>, P, S> : public Strategy<Obj<1>, P, S> */
/* { */
/* public: */
/*   void solve(std::vector<Obj<1>>& objs, std::vector<P>& procs, S& solution, */
/*              bool objsSorted) */
/*   { */
/*     if (!objsSorted) std::sort(objs.begin(), objs.end(), CmpLoadGreater<Obj<1>>()); */
/*     std::priority_queue<P, std::vector<P>, CmpLoadGreater<P>> procHeap( */
/*         CmpLoadGreater<P>(), procs); */

/*     for (const auto& o : objs) */
/*     { */
/*       P p = procHeap.top(); */
/*       procHeap.pop(); */
/*       solution.assign(o, p);  // update solution (assumes solution updates processor load) */
/*       procHeap.push(p); */
/*     } */
/*   } */
/* }; */


#endif // GREEDY_H_
