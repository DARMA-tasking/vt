#ifndef KD_H
#define KD_H

#include <array>
#ifdef DEBUG
#  include <iostream>
#endif
#include <limits>
#include <random>
#include <stack>
#include <utility>

using KDFloatType = double;

template <typename TreeType, typename Elem, int N=Elem::dimension>
class BaseKDNode
{
protected:
  Elem data;
  unsigned int size = 1;
  TreeType* left = nullptr;
  TreeType* right = nullptr;
  KDFloatType norm;

  BaseKDNode(const Elem& key) : data(key), norm(calcNorm(data)) {}

  template <typename T>
  static KDFloatType calcNorm(const T& x)
  {
    KDFloatType sum = 0;
    for (int i = 0; i < N; i++)
    {
      const auto element = x[i];
      const auto elementSq = element * element;
      sum += elementSq * elementSq;
    }
    return sum;
  }

  template <typename A, typename B>
  static KDFloatType calcNorm(const A& a, const B& b)
  {
    KDFloatType sum = 0;
    for (int i = 0; i < N; i++)
    {
      const auto element = a[i] + b[i];
      const auto elementSq = element * element;
      sum += elementSq * elementSq;
    }
    return sum;
  }

  template <typename A, typename B>
  static std::array<KDFloatType, N> addVecs(const A& a, const B& b)
  {
    std::array<KDFloatType, N> sum = {0};
    for (int i = 0; i < N; i++)
    {
      sum[i] = a[i] + b[i];
    }

    return sum;
  }

#ifdef DEBUG
public:
  static void printTree(TreeType tree, std::string prefix = "")
  {
    std::cout << prefix;
    for (int i = 0; i < N; i++) std::cout << tree->data[i] << " ";
    std::cout << std::endl;
    if (tree->left != nullptr) printTree(tree->left, prefix + "L  ");
    if (tree->right != nullptr) printTree(tree->right, prefix + "R  ");
  }
#endif
};

template <typename Elem, int N=Elem::dimension>
class KDNode : BaseKDNode<KDNode<Elem, N>, Elem>
{
  using base = BaseKDNode<KDNode<Elem, N>, Elem>;
  using kdt = KDNode*;
public:
  KDNode(const Elem& key) : base(key) {}

  static kdt insert(kdt t, const Elem& x, unsigned int depth = 0)
  {
    if (t == nullptr)
      t = new KDNode(x);
    else
    {
      t->size++;
      const auto dim = depth % N;
      if (x[dim] < t->data[dim])
	t->left = insert(t->left, x, depth + 1);
      else
	t->right = insert(t->right, x, depth + 1);
    }
    return t;
  }

  static Elem findMin(kdt t, unsigned int targetDim, unsigned int depth = 0)
  {
    assert(t != nullptr);
    const auto dim = depth % N;
    if (dim == targetDim)
    {
      if (t->left == nullptr)
	return t->data;
      else
	return findMin(t->left, targetDim, depth + 1);
    }
    else
    {
      auto obj = t->data;
      if (t->left != nullptr)
      {
	const auto left = findMin(t->left, targetDim, depth + 1);
	if (left[targetDim] < obj[targetDim])
	  obj = left;
      }
      if (t->right != nullptr)
      {
	const auto right = findMin(t->right, targetDim, depth + 1);
	if (right[targetDim] < obj[targetDim])
	  obj = right;
      }
      return obj;
    }
  }

  static kdt remove(kdt t, const Elem& x, unsigned int depth = 0)
  {
    const auto dim = depth % N;
    t->size--;

    if (t->data == x)
    {
      if (t->right != nullptr)
      {
	t->data = findMin(t->right, dim, depth + 1);
	t->right = remove(t->right, t->data, depth + 1);
      }
      else if (t->left != nullptr)
      {
	t->data = findMin(t->left, dim, depth + 1);
	t->right = remove(t->left, t->data, depth + 1);
	t->left = nullptr;
      }
      else
      {
	delete t;
	t = nullptr;
      }
    }
    else if (x[dim] < t->data[dim])
      t->left = remove(t->left, x, depth + 1);
    else
      t->right = remove(t->right, x, depth + 1);

    return t;
  }

  template <typename T>
  static Elem* findMinNorm(kdt t, const T& x)
  {
    std::array<KDFloatType, N> mins = {0};
    KDFloatType bestNorm = std::numeric_limits<KDFloatType>::max();
    return findMinNormHelper(t, x, 0, nullptr, bestNorm, mins);
  }

private:
  template <typename T>
  static Elem* findMinNormHelper(kdt t, const T& x, unsigned int depth, Elem* bestObj,
                                 KDFloatType& bestNorm, std::array<KDFloatType, N>& minBounds)
  {
    const auto dim = depth % N;

    if (t->left != nullptr)
    {
      bestObj = findMinNormHelper(t->left, x, depth + 1, bestObj, bestNorm, minBounds);
    }
    if (t->norm < bestNorm)
    {
      const auto rootNorm = base::calcNorm(x, t->data);
      if (rootNorm < bestNorm)
      {
	bestObj = &(t->data);
	bestNorm = rootNorm;
      }
    }
    if (t->right != nullptr)
    {
      const auto oldMin = minBounds[dim];
      minBounds[dim] = t->data[dim];
      if (base::calcNorm(x, minBounds) < bestNorm)
      {
	bestObj = findMinNormHelper(t->right, x, depth + 1, bestObj, bestNorm, minBounds);
      }
      minBounds[dim] = oldMin;
    }

    return bestObj;
  }
};

template <typename Elem, int N = Elem::dimension,
          typename Engine = std::default_random_engine>
class RKDNode : BaseKDNode<RKDNode<Elem, N, Engine>, Elem>
{
  using base = BaseKDNode<RKDNode<Elem, N, Engine>, Elem>;
  using rkdt = RKDNode*;
private:
  int discr;

  static int random(int min, int max)
  {
    static Engine rng;
    return std::uniform_int_distribution<int>(min, max)(rng);
  }

public:
  RKDNode(const Elem& key) : base(key), discr(random(0, N - 1)) {}

  static rkdt insert(rkdt t, const Elem& x)
  {
    std::stack<std::pair<rkdt, bool>> stack;
    while (t != nullptr && random(0, t->size) > 0)
    {
      t->size++;
      const int i = t->discr;
      const bool isLeftChild = x[i] < t->data[i];
      stack.emplace(t, isLeftChild);
      if (isLeftChild)
      {
        t = t->left;
      }
      else
      {
        t = t->right;
      }
    }

    t = insert_at_root(t, x);

    while (!stack.empty())
    {
      const std::pair<rkdt, bool>& entry = stack.top();
      const rkdt parent = entry.first;
      const bool isLeftChild = entry.second;
      if (isLeftChild)
        parent->left = t;
      else
        parent->right = t;
      t = parent;
      stack.pop();
    }
    return t;
  }

  static rkdt remove(rkdt t, const Elem& x)
  {
    if (t == nullptr) return nullptr;
    const int i = t->discr;
    if (t->data == x)
    {
      const auto newRoot = join(t->left, t->right, i);
      delete t;
      return newRoot;
    }
    t->size--;
    if (x[i] < t->data[i])
      t->left = remove(t->left, x);
    else
      t->right = remove(t->right, x);
    return t;
  }

  static rkdt insert_at_root(rkdt t, const Elem& x)
  {
    rkdt r = new RKDNode(x);
    if (t != nullptr) r->size += t->size;
    auto p = split(t, r);
    r->left = p.first;
    r->right = p.second;
    return r;
  }

  // This splits t's subtrees at whatever r specifies
  // t might be nullptr, r cannot be
  static std::pair<rkdt, rkdt> split(rkdt t, rkdt r)
  {
    if (t == nullptr) return std::make_pair(nullptr, nullptr);
    const int i = r->discr;
    const int j = t->discr;

    // t and r discriminate in the same dimension, so just resplit the appropriate subtree
    // of t
    if (i == j)
    {
      if (r->data[i] < t->data[i])
      {
        const auto p = split(t->left, r);
        t->left = p.second;
        const int splitSize = (p.first == nullptr) ? 0 : p.first->size;
        t->size -= splitSize;
        return std::make_pair(p.first, t);
      }
      else
      {
        const auto p = split(t->right, r);
        t->right = p.first;
        const int splitSize = (p.second == nullptr) ? 0 : p.second->size;
        t->size -= splitSize;
        return std::make_pair(t, p.second);
      }
    }
    // t and r discriminate on different dimensions, so recursively split both subtrees of
    // t
    else
    {
      const auto L = split(t->left, r);
      const auto R = split(t->right, r);
      if (r->data[i] < t->data[i])
      {
        t->left = L.second;
        t->right = R.second;
        const int splitSize = ((L.first == nullptr) ? 0 : L.first->size) +
                              ((R.first == nullptr) ? 0 : R.first->size);
        t->size -= splitSize;

        return std::make_pair(join(L.first, R.first, j), t);
      }
      else
      {
        t->left = L.first;
        t->right = R.first;
        const int splitSize = ((L.second == nullptr) ? 0 : L.second->size) +
                              ((R.second == nullptr) ? 0 : R.second->size);
        t->size -= splitSize;

        return std::make_pair(t, join(L.second, R.second, j));
      }
    }
  }

  static rkdt join(rkdt l, rkdt r, int dim)
  {
    if (l == nullptr) return r;
    if (r == nullptr) return l;

    const int m = l->size;
    const int n = r->size;
    const int u = random(0, m + n - 1);
    if (u < m)
    {
      l->size += r->size;
      if (l->discr == dim)
      {
        l->right = join(l->right, r, dim);
        return l;
      }
      else
      {
        auto R = split(r, l);
        l->left = join(l->left, R.first, dim);
        l->right = join(l->right, R.second, dim);
        return l;
      }
    }
    else
    {
      r->size += l->size;
      if (r->discr == dim)
      {
        r->left = join(l, r->left, dim);
        return r;
      }
      else
      {
        auto L = split(l, r);
        r->left = join(L.first, r->left, dim);
        r->right = join(L.second, r->right, dim);
        return r;
      }
    }
  }

  template <typename T>
  static Elem* findMinNorm(rkdt t, const T& x)
  {
    std::array<KDFloatType, N> mins = {0};
    KDFloatType bestNorm = std::numeric_limits<KDFloatType>::max();
    return findMinNormHelper(t, x, nullptr, bestNorm, mins);
  }

private:
  template <typename T>
  static Elem* findMinNormHelper(rkdt t, const T& x, Elem* bestObj, KDFloatType& bestNorm,
                                 std::array<KDFloatType, N>& minBounds)
  {
    const auto dim = t->discr;

    if (t->left != nullptr)
    {
      bestObj = findMinNormHelper(t->left, x, bestObj, bestNorm, minBounds);
    }
    if (t->norm < bestNorm)
    {
      const auto rootNorm = base::calcNorm(x, t->data);
      if (rootNorm < bestNorm)
      {
        bestObj = &(t->data);
        bestNorm = rootNorm;
      }
    }
    if (t->right != nullptr)
    {
      const auto oldMin = minBounds[dim];
      minBounds[dim] = t->data[dim];
      if (base::calcNorm(x, minBounds) < bestNorm)
      {
        bestObj = findMinNormHelper(t->right, x, bestObj, bestNorm, minBounds);
      }
      minBounds[dim] = oldMin;
    }

    return bestObj;
  }

  static Elem findMin(rkdt t, unsigned int targetDim)
  {
    assert(t != nullptr);
    const auto dim = t->discr;
    if (dim == targetDim)
    {
      if (t->left == nullptr)
        return t->data;
      else
        return findMin(t->left, targetDim);
    }
    else
    {
      auto obj = t->data;
      if (t->left != nullptr)
      {
        const auto left = findMin(t->left, targetDim);
        if (left[targetDim] < obj[targetDim]) obj = left;
      }
      if (t->right != nullptr)
      {
        const auto right = findMin(t->right, targetDim);
        if (right[targetDim] < obj[targetDim]) obj = right;
      }
      return obj;
    }
  }
};

#endif /* KD_H */
