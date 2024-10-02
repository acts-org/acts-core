// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <cmath>
#include <exception>
#include <functional>
#include <sstream>
#include <vector>

#include "TDictionary.h"
#include "TTreeReaderValue.h"

// Pairs of elements of the same type
template <typename T>
using HomogeneousPair = std::pair<T, T>;

// === TYPE ERASURE FOR CONCRETE DATA ===

// Minimal type-erasure wrapper for std::vector<T>. This will be used as a
// workaround to compensate for the absence of C++17's std::any in Cling.
class AnyVector {
 public:
  // Create a type-erased vector<T>, using proposed constructor arguments.
  // Returns a pair containing the type-erased vector and a pointer to the
  // underlying concrete vector.
  template <typename T, typename... Args>
  static std::pair<AnyVector, std::vector<T>*> create(Args&&... args) {
    std::vector<T>* vector = new std::vector<T>(std::forward<Args>(args)...);
    std::function<void()> deleter = [vector] { delete vector; };
    return std::make_pair(
        AnyVector{static_cast<void*>(vector), std::move(deleter)}, vector);
  }

  // Default-construct a null type-erased vector
  AnyVector() = default;

  // Move-construct a type-erased vector
  AnyVector(AnyVector&& other)
      : m_vector{other.m_vector}, m_deleter{std::move(other.m_deleter)} {
    other.m_vector = nullptr;
  }

  // Move-assign a type-erased vector
  AnyVector& operator=(AnyVector&& other) {
    if (&other != this) {
      m_vector = other.m_vector;
      m_deleter = std::move(other.m_deleter);
      other.m_vector = nullptr;
    }
    return *this;
  }

  // Forbid copies of type-erased vectors
  AnyVector(const AnyVector&) = delete;
  AnyVector& operator=(const AnyVector&) = delete;

  // Delete a type-erased vector
  ~AnyVector() {
    if (m_vector != nullptr) {
      m_deleter();
    }
  }

 private:
  // Construct a type-erased vector from a concrete vector
  AnyVector(void* vector, std::function<void()>&& deleter)
      : m_vector{vector}, m_deleter{std::move(deleter)} {}

  void* m_vector{nullptr};          // Casted std::vector<T>*
  std::function<void()> m_deleter;  // Deletes the underlying vector
};

// === GENERIC DATA ORDERING ===

// We want to check, in a single operation, how two pieces of data are ordered
enum class Ordering { SMALLER, EQUAL, GREATER };

// In general, any type which implements comparison operators that behave as a
// mathematical total order can use this comparison function...
template <typename T>
Ordering compare(const T& x, const T& y) {
  if (x < y) {
    return Ordering::SMALLER;
  } else if (x == y) {
    return Ordering::EQUAL;
  } else {
    return Ordering::GREATER;
  }
}

// ...but we'll want to tweak that a little for floats, to handle NaNs better...
template <typename T>
Ordering compareFloat(const T& x, const T& y) {
  if (std::isless(x, y)) {
    return Ordering::SMALLER;
  } else if (std::isgreater(x, y)) {
    return Ordering::GREATER;
  } else {
    return Ordering::EQUAL;
  }
}

template <>
Ordering compare(const float& x, const float& y) {
  return compareFloat(x, y);
}

template <>
Ordering compare(const double& x, const double& y) {
  return compareFloat(x, y);
}

// ...and for vectors, where the default lexicographic comparison cannot
// efficiently tell all of what we want in a single vector iteration pass.
template <typename U>
Ordering compare(const std::vector<U>& v1, const std::vector<U>& v2) {
  // First try to order by size...
  if (v1.size() < v2.size()) {
    return Ordering::SMALLER;
  } else if (v1.size() > v2.size()) {
    return Ordering::GREATER;
  }
  // ...if the size is identical...
  else {
    // ...then try to order by contents of increasing index...
    for (std::size_t i = 0; i < v1.size(); ++i) {
      if (v1[i] < v2[i]) {
        return Ordering::SMALLER;
      } else if (v1[i] > v2[i]) {
        return Ordering::GREATER;
      }
    }

    // ...and declare the vectors equal if the contents are equal
    return Ordering::EQUAL;
  }
}

// === GENERIC SORTING MECHANISM ===

// The following functions are generic implementations of sorting algorithms,
// which require only a comparison operator, a swapping operator, and an
// inclusive range of indices to be sorted in order to operate
using IndexComparator = std::function<Ordering(std::size_t, std::size_t)>;
using IndexSwapper = std::function<void(std::size_t, std::size_t)>;

// Selection sort has pertty bad asymptotic scaling, but it is non-recursive
// and in-place, which makes it a good choice for smaller inputs
void selectionSort(const std::size_t firstIndex, const std::size_t lastIndex,
                   const IndexComparator& compare, const IndexSwapper& swap) {
  for (std::size_t targetIndex = firstIndex; targetIndex < lastIndex;
       ++targetIndex) {
    std::size_t minIndex = targetIndex;
    for (std::size_t readIndex = targetIndex + 1; readIndex <= lastIndex;
         ++readIndex) {
      if (compare(readIndex, minIndex) == Ordering::SMALLER) {
        minIndex = readIndex;
      }
    }
    if (minIndex != targetIndex) {
      swap(minIndex, targetIndex);
    }
  }
}

// Quick sort is used as the top-level sorting algorithm for our datasets
void quickSort(const std::size_t firstIndex, const std::size_t lastIndex,
               const IndexComparator& compare, const IndexSwapper& swap) {
  // We switch to non-recursive selection sort when the range becomes too small.
  // This optimization voids the need for detection of 0- and 1-element input.
  static const std::size_t NON_RECURSIVE_THRESHOLD = 25;
  if (lastIndex - firstIndex < NON_RECURSIVE_THRESHOLD) {
    selectionSort(firstIndex, lastIndex, compare, swap);
    return;
  }

  // We'll use the midpoint as a pivot. Later on, we can switch to more
  // elaborate pivot selection schemes if their usefulness for our use case
  // (pseudorandom events with thread-originated reordering) is demonstrated.
  std::size_t pivotIndex = firstIndex + (lastIndex - firstIndex) / 2;

  // Partition the data around the pivot using Hoare's scheme
  std::size_t splitIndex = 0;
  {
    // Start with two indices one step beyond each side of the array
    std::size_t i = firstIndex - 1;
    std::size_t j = lastIndex + 1;
    while (true) {
      // Move left index forward at least once, and until an element which is
      // greater than or equal to the pivot is detected.
      do {
        i = i + 1;
      } while (compare(i, pivotIndex) == Ordering::SMALLER);

      // Move right index backward at least once, and until an element which is
      // smaller than or equal to the pivot is detected
      do {
        j = j - 1;
      } while (compare(j, pivotIndex) == Ordering::GREATER);

      // By transitivity of inequality, the element at location i is greater
      // than or equal to the one at location j, and a swap could be required
      if (i < j) {
        // These elements are in the wrong order, swap them
        swap(i, j);

        // Don't forget to keep track the pivot's index along the way, as this
        // is currently the only way by which we can refer to the pivot element.
        if (i == pivotIndex) {
          pivotIndex = j;
        } else if (j == pivotIndex) {
          pivotIndex = i;
        }
      } else {
        // If i and j went past each other, our partitioning is done
        splitIndex = j;
        break;
      }
    }
  }

  // Now, we'll recursively sort both partitions using quicksort. We should
  // recurse in the smaller range first, so as to leverage compiler tail call
  // optimization if available.
  if (splitIndex - firstIndex <= lastIndex - splitIndex - 1) {
    quickSort(firstIndex, splitIndex, compare, swap);
    quickSort(splitIndex + 1, lastIndex, compare, swap);
  } else {
    quickSort(splitIndex + 1, lastIndex, compare, swap);
    quickSort(firstIndex, splitIndex, compare, swap);
  }
}

// === GENERIC TTREE BRANCH MANIPULATION MECHANISM ===

// When comparing a pair of TTrees, we'll need to set up quite a few facilities
// for each branch. Since this setup is dependent on the branch data type, which
// is only known at runtime, it is quite involved, which is why we extracted it
// to a separate struct and its constructor.
struct BranchComparisonHarness {
  // We'll keep track of the branch name for debugging purposes
  std::string branchName;

  // Type-erased event data for the current branch, in both trees being compared
  HomogeneousPair<AnyVector> eventData;

  // Function which loads the active event data for the current branch. This is
  // to be performed for each branch and combined with TTreeReader-based event
  // iteration on both trees.
  void loadCurrentEvent() { (*m_eventLoaderPtr)(); }

  // Functors which compare two events within a given tree and order them
  // with respect to one another, and which swap two events. By combining such
  // functionality for each branch, a global tree order can be produced.
  HomogeneousPair<std::pair<IndexComparator, IndexSwapper>> sortHarness;

  // Functor which compares the current event data in *both* trees and tells
  // whether it is identical. The comparison is order-sensitive, so events
  // should previously have been sorted in a canonical order in both trees.
  // By combining the results for each branch, global tree equality is defined.
  using TreeComparator = std::function<bool()>;
  TreeComparator eventDataEqual;

  // Functor which dumps the event data for the active event side by side, in
  // two columns. This enables manual comparison during debugging.
  std::function<void()> dumpEventData;

  // General metadata about the tree which is identical for every branch
  struct TreeMetadata {
    TTreeReader& tree1Reader;
    TTreeReader& tree2Reader;
    const std::size_t entryCount;
  };

  // This exception will be thrown if an unsupported branch type is encountered
  class UnsupportedBranchType : public std::exception {};

  // Type-erased factory of branch comparison harnesses, taking ROOT run-time
  // type information as input in order to select an appropriate C++ constructor
  static BranchComparisonHarness create(TreeMetadata& treeMetadata,
                                        const std::string& branchName,
                                        const EDataType dataType,
                                        const std::string& className) {
    switch (dataType) {
      case kChar_t:
        return BranchComparisonHarness::create<char>(treeMetadata, branchName);
      case kUChar_t:
        return BranchComparisonHarness::create<unsigned char>(treeMetadata,
                                                              branchName);
      case kShort_t:
        return BranchComparisonHarness::create<short>(treeMetadata, branchName);
      case kUShort_t:
        return BranchComparisonHarness::create<unsigned short>(treeMetadata,
                                                               branchName);
      case kInt_t:
        return BranchComparisonHarness::create<int>(treeMetadata, branchName);
      case kUInt_t:
        return BranchComparisonHarness::create<unsigned int>(treeMetadata,
                                                             branchName);
      case kLong_t:
        return BranchComparisonHarness::create<long>(treeMetadata, branchName);
      case kULong_t:
        return BranchComparisonHarness::create<unsigned long>(treeMetadata,
                                                              branchName);
      case kULong64_t:
        return BranchComparisonHarness::create<unsigned long long>(treeMetadata,
                                                                   branchName);

      case kFloat_t:
        return BranchComparisonHarness::create<float>(treeMetadata, branchName);
      case kDouble_t:
        return BranchComparisonHarness::create<double>(treeMetadata,
                                                       branchName);
      case kBool_t:
        return BranchComparisonHarness::create<bool>(treeMetadata, branchName);
      case kOther_t:
        if (className.substr(0, 6) == "vector") {
          std::string elementType = className.substr(7, className.size() - 8);
          return BranchComparisonHarness::createVector(treeMetadata, branchName,
                                                       std::move(elementType));
        } else {
          throw UnsupportedBranchType();
        }
      default:
        throw UnsupportedBranchType();
    }
  }

 private:
  // Under the hood, the top-level factory calls the following function
  // template, parametrized with the proper C++ data type
  template <typename T>
  static BranchComparisonHarness create(TreeMetadata& treeMetadata,
                                        const std::string& branchName) {
    // Our result will eventually go there
    BranchComparisonHarness result;

    // Save the branch name for debugging purposes
    result.branchName = branchName;

    // Setup type-erased event data storage
    auto tree1DataStorage = AnyVector::create<T>();
    auto tree2DataStorage = AnyVector::create<T>();
    result.eventData = std::make_pair(std::move(tree1DataStorage.first),
                                      std::move(tree2DataStorage.first));
    std::vector<T>& tree1Data = *tree1DataStorage.second;
    std::vector<T>& tree2Data = *tree2DataStorage.second;

    // Use our advance knowledge of the event count to preallocate storage
    tree1Data.reserve(treeMetadata.entryCount);
    tree2Data.reserve(treeMetadata.entryCount);

    // Setup event data readout
    result.m_eventLoaderPtr.reset(
        new EventLoaderT<T>{treeMetadata.tree1Reader, treeMetadata.tree2Reader,
                            branchName, tree1Data, tree2Data});

    // Setup event comparison and swapping for each tree
    result.sortHarness = std::make_pair(
        std::make_pair(
            [&tree1Data](std::size_t i, std::size_t j) -> Ordering {
              return compare(tree1Data[i], tree1Data[j]);
            },
            [&tree1Data](std::size_t i, std::size_t j) {
              std::swap(tree1Data[i], tree1Data[j]);
            }),
        std::make_pair(
            [&tree2Data](std::size_t i, std::size_t j) -> Ordering {
              return compare(tree2Data[i], tree2Data[j]);
            },
            [&tree2Data](std::size_t i, std::size_t j) {
              std::swap(tree2Data[i], tree2Data[j]);
            }));

    // Setup order-sensitive tree comparison
    result.eventDataEqual = [&tree1Data, &tree2Data]() -> bool {
      for (std::size_t i = 0; i < tree1Data.size(); ++i) {
        if (compare(tree1Data[i], tree2Data[i]) != Ordering::EQUAL) {
          return false;
        }
      }
      return true;
    };

    // Add a debugging method to dump event data
    result.dumpEventData = [&tree1Data, &tree2Data] {
      std::cout << "File 1                \tFile 2" << std::endl;
      for (std::size_t i = 0; i < tree1Data.size(); ++i) {
        std::cout << toString(tree1Data[i]) << "      \t"
                  << toString(tree2Data[i]) << std::endl;
      }
    };

    // ...and we're good to go!
    return std::move(result);
  }

  // Because the people who created TTreeReaderValue could not bother to make it
  // movable (for moving it into a lambda), or even just virtually destructible
  // (for moving a unique_ptr into the lambda), loadEventData can only be
  // implemented through lots of unpleasant C++98-ish boilerplate.
  class IEventLoader {
   public:
    virtual ~IEventLoader() = default;
    virtual void operator()() = 0;
  };

  template <typename T>
  class EventLoaderT : public IEventLoader {
   public:
    EventLoaderT(TTreeReader& tree1Reader, TTreeReader& tree2Reader,
                 const std::string& branchName, std::vector<T>& tree1Data,
                 std::vector<T>& tree2Data)
        : branch1Reader{tree1Reader, branchName.c_str()},
          branch2Reader{tree2Reader, branchName.c_str()},
          branch1Data(tree1Data),
          branch2Data(tree2Data) {}

    void operator()() override {
      branch1Data.push_back(*branch1Reader);
      branch2Data.push_back(*branch2Reader);
    }

   private:
    TTreeReaderValue<T> branch1Reader, branch2Reader;
    std::vector<T>& branch1Data;
    std::vector<T>& branch2Data;
  };

  std::unique_ptr<IEventLoader> m_eventLoaderPtr;

  // This helper factory helps building branches associated with std::vectors
  // of data, which are the only STL collection that we support at the moment.
  static BranchComparisonHarness createVector(TreeMetadata& treeMetadata,
                                              const std::string& branchName,
                                              const std::string elemType) {
// We support vectors of different types by switching across type (strings)
#define CREATE_VECTOR__HANDLE_TYPE(type_name)                       \
  if (elemType == #type_name) {                                     \
    return BranchComparisonHarness::create<std::vector<type_name>>( \
        treeMetadata, branchName);                                  \
  }

    // Handle vectors of booleans
    CREATE_VECTOR__HANDLE_TYPE(bool)

    // Handle vectors of all standard floating-point types
    else CREATE_VECTOR__HANDLE_TYPE(float) else CREATE_VECTOR__HANDLE_TYPE(
        double)

// For integer types, we'll want to handle both signed and unsigned versions
#define CREATE_VECTOR__HANDLE_INTEGER_TYPE(integer_type_name) \
  CREATE_VECTOR__HANDLE_TYPE(integer_type_name)               \
  else CREATE_VECTOR__HANDLE_TYPE(unsigned integer_type_name)

        // Handle vectors of all standard integer types
        else CREATE_VECTOR__HANDLE_INTEGER_TYPE(char) else CREATE_VECTOR__HANDLE_INTEGER_TYPE(short) else CREATE_VECTOR__HANDLE_INTEGER_TYPE(
            int) else CREATE_VECTOR__HANDLE_INTEGER_TYPE(long) else CREATE_VECTOR__HANDLE_INTEGER_TYPE(long long)

        // Throw an exception if the vector element type is not recognized
        else throw UnsupportedBranchType();
  }

  // This helper method provides general string conversion for all supported
  // branch event data types.
  template <typename T>
  static std::string toString(const T& data) {
    std::ostringstream oss;
    oss << data;
    return oss.str();
  }

  template <typename U>
  static std::string toString(const std::vector<U>& vector) {
    std::ostringstream oss{"{ "};
    for (const auto& data : vector) {
      oss << data << "  \t";
    }
    oss << " }";
    return oss.str();
  }
};
