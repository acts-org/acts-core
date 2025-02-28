// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include <optional>

#include <tbb/parallel_for.h>
#include <tbb/queuing_mutex.h>
#include <tbb/task_arena.h>

/// Wrapper for most of the tbb functions that we use in Sequencer.
///
/// It disables the use of tbb if nthreads=1.
/// Note that only a small subset of tbb functions are implemented, and
/// tbb::blocked_range (which doesn't require any thread setup) is still taken
/// from the tbb library.
///
/// Based on an idea from
///   https://stackoverflow.com/questions/59736661/how-to-completely-switch-off-threading-in-tbb-code

namespace ActsExamples::tbbWrap {
/// enableTBB keeps a record of whether we are multi-threaded (nthreads!=1) or
/// not. This is set once in task_arena and stored globally.
/// This means that enableTBB(nthreads) itself is not thread-safe. That should
/// be fine because the task_arena is initialised before spawning any threads.
/// If multi-threading is ever enabled, then it is not disabled.
static bool enableTBB(int nthreads = -99) {
  static bool setting = false;
  if (nthreads != -99) {
    bool newSetting = (nthreads != 1);
    if (!setting && newSetting) {
      setting = newSetting;
    }
  }
  return setting;
}

/// Small wrapper for tbb::task_arena.
/// Note that the tbbWrap::task_arena constructor is not thread-safe.
/// That should be fine because the task_arena is initialised before spawning
/// any threads.
class task_arena {
  std::optional<tbb::task_arena> tbb;

 public:
  explicit task_arena(int nthreads = tbb::task_arena::automatic,
                      unsigned res = 1) {
    if (enableTBB(nthreads)) {
      tbb.emplace(nthreads, res);
    }
  }

  template <typename F>
  void execute(const F& f) {
    if (tbb) {
      tbb->execute(f);
    } else {
      f();
    }
  }
};

/// Small wrapper for tbb::parallel_for.
class parallel_for {
 public:
  template <typename R, typename F>
  parallel_for(const R& r, const F& f) {
    if (enableTBB()) {
      tbb::parallel_for(r, f);
    } else {
      for (auto i = r.begin(); i != r.end(); ++i) {  // use default grainsize=1
        f(R(i, i + 1));
      }
    }
  }
};

/// Small wrapper for tbb::queuing_mutex and tbb::queuing_mutex::scoped_lock.
class queuing_mutex {
  std::optional<tbb::queuing_mutex> tbb;

 public:
  queuing_mutex() {
    if (enableTBB()) {
      tbb.emplace();
    }
  }

  class scoped_lock {
    std::optional<tbb::queuing_mutex::scoped_lock> tbb;

   public:
    scoped_lock() {
      if (enableTBB()) {
        tbb.emplace();
      }
    }

    explicit scoped_lock(queuing_mutex& m) {
      if (enableTBB()) {
        tbb.emplace(*m.tbb);
      }
    }
  };
};

}  // namespace ActsExamples::tbbWrap
