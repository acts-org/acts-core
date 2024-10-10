// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

// uncomment to remove all use of tbb library.
// #define ACTS_EXAMPLES_NO_TBB

#ifdef ACTS_EXAMPLES_NO_TBB
#define ACTS_EXAMPLES_WITH_TBB(a)
#include <stdexcept>
#else
#define ACTS_EXAMPLES_WITH_TBB(a) a
#include <optional>

#include <tbb/parallel_for.h>
#include <tbb/queuing_mutex.h>
#include <tbb/task_arena.h>
#endif

/// Wrapper for most of the tbb functions that we use in Sequencer.
///
/// It disables the use of tbb if nthreads=1.
/// Note that only a small subset of tbb functions are implemented, and
/// tbb::blocked_range (which doesn't require any thread setup) is still taken
/// from the tbb library.
///
/// However, if ACTS_EXAMPLES_NO_TBB is defined, then don't use tbb library at
/// all (requires nthreads=1 or -1). This allows the ACTS Examples to be built
/// without the tbb library (and reduces the dependency on ROOT).
/// In this case, we provide our own minimal implementation of
/// tbb::blocked_range.
///
/// Based on an idea from
///   https://stackoverflow.com/questions/59736661/how-to-completely-switch-off-threading-in-tbb-code

#ifdef ACTS_EXAMPLES_NO_TBB
namespace ActsExamples::tbb {
namespace task_arena {
constexpr int automatic = -1;
}  // namespace task_arena

template <typename Value>
struct blocked_range {
  blocked_range(Value begin_, Value end_) : my_end(end_), my_begin(begin_) {}
  Value begin() const { return my_begin; }
  Value end() const { return my_end; }

 private:
  Value my_end;
  Value my_begin;
};
}  // namespace ActsExamples::tbb
#endif

namespace ActsExamples::tbbWrap {
/// enableTBB keeps a record of whether we are multi-threaded (nthreads!=1) or
/// not. This is set once in task_arena and stored globally.
/// This means that enableTBB(nthreads) itself is not thread-safe. That should
/// be fine because the task_arena is initialised before spawning any threads.
/// If multi-threading is ever enabled, then it is not disabled.
static bool enableTBB(int nthreads = -99) {
  static bool setting = false;
  if (nthreads != -99) {
#ifdef ACTS_EXAMPLES_NO_TBB
    if (nthreads > 1) {
      throw std::runtime_error(
          "tbb is not available, so can't do multi-threading.");
    }
#else
    bool newSetting = (nthreads != 1);
    if (!setting && newSetting) {
      setting = newSetting;
    }
#endif
  }
  return setting;
}

/// Small wrapper for tbb::task_arena.
/// Note that the tbbWrap::task_arena constructor is not thread-safe.
/// That should be fine because the task_arena is initialised before spawning
/// any threads.
class task_arena {
#ifndef ACTS_EXAMPLES_NO_TBB
  std::optional<tbb::task_arena> tbb;
#endif

 public:
  task_arena(int nthreads = tbb::task_arena::automatic,
             unsigned ACTS_EXAMPLES_WITH_TBB(res) = 1) {
    if (enableTBB(nthreads)) {
#ifndef ACTS_EXAMPLES_NO_TBB
      tbb.emplace(nthreads, res);
#endif
    }
  }

  template <typename F>
  void execute(const F& f) {
#ifndef ACTS_EXAMPLES_NO_TBB
    if (tbb) {
      tbb->execute(f);
    } else
#endif
    {
      f();
    }
  }
};

/// Small wrapper for tbb::parallel_for.
class parallel_for {
 public:
  template <typename R, typename F>
  parallel_for(const R& r, const F& f) {
#ifndef ACTS_EXAMPLES_NO_TBB
    if (enableTBB()) {
      tbb::parallel_for(r, f);
    } else
#endif
    {
      for (auto i = r.begin(); i != r.end(); ++i) {  // use default grainsize=1
        f(R(i, i + 1));
      }
    }
  }
};

/// Small wrapper for tbb::queuing_mutex and tbb::queuing_mutex::scoped_lock.
class queuing_mutex {
#ifndef ACTS_EXAMPLES_NO_TBB
  std::optional<tbb::queuing_mutex> tbb;
#endif

 public:
  queuing_mutex() {
#ifndef ACTS_EXAMPLES_NO_TBB
    if (enableTBB()) {
      tbb.emplace();
    }
#endif
  }

  class scoped_lock {
#ifndef ACTS_EXAMPLES_NO_TBB
    std::optional<tbb::queuing_mutex::scoped_lock> tbb;
#endif

   public:
    scoped_lock() {
#ifndef ACTS_EXAMPLES_NO_TBB
      if (enableTBB()) {
        tbb.emplace();
      }
#endif
    }

    scoped_lock(queuing_mutex& ACTS_EXAMPLES_WITH_TBB(m)) {
#ifndef ACTS_EXAMPLES_NO_TBB
      if (enableTBB()) {
        tbb.emplace(*m.tbb);
      }
#endif
    }
  };
};

}  // namespace ActsExamples::tbbWrap
