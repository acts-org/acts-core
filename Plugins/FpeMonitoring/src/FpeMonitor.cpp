// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Plugins/FpeMonitoring/FpeMonitor.hpp"

#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <bitset>
#include <cfenv>
#include <csignal>
#include <cstddef>
#include <cstdint>
#include <iostream>
#include <iterator>
#include <memory>
#include <mutex>
#include <stdexcept>
#include <string_view>
#include <vector>

#include <boost/stacktrace/frame.hpp>
#include <boost/stacktrace/safe_dump_to.hpp>
#include <boost/stacktrace/stacktrace.hpp>
#include <boost/stacktrace/stacktrace_fwd.hpp>

#define FPU_EXCEPTION_MASK 0x3f
#define FPU_STATUS_FLAGS 0xff
#define SSE_STATUS_FLAGS FPU_EXCEPTION_MASK
#define SSE_EXCEPTION_MASK (FPU_EXCEPTION_MASK << 7)

namespace Acts {

namespace {
bool areFpesEquivalent(
    std::pair<FpeType, const boost::stacktrace::stacktrace &> lhs,
    std::pair<FpeType, const boost::stacktrace::stacktrace &> rhs) {
  const auto &fl = *lhs.second.begin();
  const auto &fr = *rhs.second.begin();
  return lhs.first == rhs.first && (boost::stacktrace::hash_value(fl) ==
                                    boost::stacktrace::hash_value(fr));
}
}  // namespace

FpeMonitor::Result::FpeInfo::~FpeInfo() = default;

FpeMonitor::Result::FpeInfo::FpeInfo(
    std::size_t countIn, FpeType typeIn,
    std::shared_ptr<const boost::stacktrace::stacktrace> stIn)
    : count{countIn}, type{typeIn}, st{std::move(stIn)} {}

FpeMonitor::Result FpeMonitor::Result::merged(const Result &with) const {
  Result result{};

  for (unsigned int i = 0; i < m_counts.size(); i++) {
    result.m_counts[i] = m_counts[i] + with.m_counts[i];
  }

  std::copy(with.m_stackTraces.begin(), with.m_stackTraces.end(),
            std::back_inserter(result.m_stackTraces));
  std::copy(m_stackTraces.begin(), m_stackTraces.end(),
            std::back_inserter(result.m_stackTraces));

  result.deduplicate();

  return result;
}

void FpeMonitor::Result::merge(const Result &with) {
  for (unsigned int i = 0; i < m_counts.size(); i++) {
    m_counts[i] = m_counts[i] + with.m_counts[i];
  }

  std::copy(with.m_stackTraces.begin(), with.m_stackTraces.end(),
            std::back_inserter(m_stackTraces));

  deduplicate();
}

void FpeMonitor::Result::add(FpeType type, void *stackPtr,
                             std::size_t bufferSize) {
  auto st = std::make_unique<boost::stacktrace::stacktrace>(
      boost::stacktrace::stacktrace::from_dump(stackPtr, bufferSize));

  auto it = std::ranges::find_if(m_stackTraces, [&](const FpeInfo &el) {
    return areFpesEquivalent({el.type, *el.st}, {type, *st});
  });

  if (it != m_stackTraces.end()) {
    it->count += 1;
  } else {
    m_stackTraces.push_back({1, type, std::move(st)});
  }
}

bool FpeMonitor::Result::contains(
    FpeType type, const boost::stacktrace::stacktrace &st) const {
  return std::ranges::any_of(m_stackTraces, [&](const FpeInfo &el) {
    return areFpesEquivalent({el.type, *el.st}, {type, st});
  });
}

FpeMonitor::Result &FpeMonitor::result() {
  consumeRecorded();
  return m_result;
}

void FpeMonitor::consumeRecorded() {
  if (m_recorded.empty()) {
    return;
  }

  for (auto [type, stackPtr, remaining] : m_recorded) {
    m_result.add(type, stackPtr, remaining);
  }

  m_buffer.reset();
  m_recorded.clear();
}

unsigned int FpeMonitor::Result::count(FpeType type) const {
  return m_counts.at(static_cast<std::uint32_t>(type));
}

unsigned int FpeMonitor::Result::numStackTraces() const {
  return m_stackTraces.size();
}

const std::vector<FpeMonitor::Result::FpeInfo> &
FpeMonitor::Result::stackTraces() const {
  return m_stackTraces;
}

bool FpeMonitor::Result::encountered(FpeType type) const {
  return count(type) > 0;
}

void FpeMonitor::Result::summary(std::ostream &os, std::size_t depth) const {
  os << "FPE result summary:\n";
  static const std::vector<FpeType> types = {
      FpeType::INTDIV, FpeType::INTOVF, FpeType::FLTDIV, FpeType::FLTOVF,
      FpeType::FLTUND, FpeType::FLTRES, FpeType::FLTINV, FpeType::FLTSUB};

  for (auto type : types) {
    os << "- " << type << ": " << count(type) << "\n";
  }

  os << "\nStack traces:\n";
  for (const auto &[count, type, st] : stackTraces()) {
    os << "- " << type << ": (" << count << " times)\n";

    os << stackTraceToString(*st, depth);
  }
  os << std::endl;
}

void FpeMonitor::Result::deduplicate() {
  std::vector<FpeInfo> copy{};
  copy = std::move(m_stackTraces);
  m_stackTraces.clear();

  for (auto &info : copy) {
    auto it = std::ranges::find_if(m_stackTraces, [&info](const FpeInfo &el) {
      return areFpesEquivalent({el.type, *el.st}, {info.type, *info.st});
    });
    if (it != m_stackTraces.end()) {
      it->count += info.count;
      continue;
    }
    m_stackTraces.push_back({info.count, info.type, std::move(info.st)});
  }
}

FpeMonitor::FpeMonitor()
    : m_excepts{FE_DIVBYZERO | FE_INVALID | FE_OVERFLOW | FE_UNDERFLOW} {
  enable();
}

FpeMonitor::FpeMonitor(int excepts) : m_excepts(excepts) {
  enable();
}

FpeMonitor::~FpeMonitor() {
  disable();
}

void FpeMonitor::signalHandler(int /*signal*/, siginfo_t *si, void *ctx) {
  if (stack().empty()) {
    return;
  }

  FpeMonitor &fpe = *stack().top();
  fpe.m_result.m_counts.at(si->si_code)++;

  try {
    // collect stack trace skipping 2 frames, which should be the signal handler
    // and the calling facility. This might be platform specific, not sure
    auto [buffer, remaining] = fpe.m_buffer.next();
    std::size_t depth = boost::stacktrace::safe_dump_to(2, buffer, remaining);
    std::size_t stored =
        depth * sizeof(boost::stacktrace::frame::native_frame_ptr_t);
    fpe.m_buffer.pushOffset(stored);  // record how much storage was consumed
    fpe.m_recorded.emplace_back(
        static_cast<FpeType>(si->si_code), buffer,
        remaining);  // record buffer offset and fpe type

  } catch (const std::bad_alloc &e) {
    std::cout << "Unable to collect stack trace due to memory limit"
              << std::endl;
  }

#if defined(__linux__) && defined(__x86_64__)
  __uint16_t *cw = &(static_cast<ucontext_t *>(ctx))->uc_mcontext.fpregs->cwd;
  *cw |= FPU_EXCEPTION_MASK;

  __uint16_t *sw = &(static_cast<ucontext_t *>(ctx))->uc_mcontext.fpregs->swd;
  *sw &= ~FPU_STATUS_FLAGS;

  __uint32_t *mxcsr =
      &(static_cast<ucontext_t *>(ctx))->uc_mcontext.fpregs->mxcsr;
  // *mxcsr |= SSE_EXCEPTION_MASK;  // disable all SSE exceptions
  *mxcsr |= ((*mxcsr & SSE_STATUS_FLAGS) << 7);
  *mxcsr &= ~SSE_STATUS_FLAGS;  // clear all pending SSE exceptions
#else
  (void)ctx;
#endif
}

void FpeMonitor::enable() {
#if defined(__linux__) && defined(__x86_64__)
  ensureSignalHandlerInstalled();

  // clear pending exceptions so they don't immediately fire
  std::feclearexcept(m_excepts);

  if (!stack().empty()) {
    // unset previous except state
    fedisableexcept(stack().top()->m_excepts);
  }
  // apply this stack
  feenableexcept(m_excepts);

  stack().push(this);
#else
  (void)m_excepts;
#endif
}

void FpeMonitor::rearm() {
  consumeRecorded();
#if defined(__linux__) && defined(__x86_64__)
  std::feclearexcept(m_excepts);
  feenableexcept(m_excepts);
#endif
}

void FpeMonitor::ensureSignalHandlerInstalled() {
  auto &state = globalState();
  if (state.isSignalHandlerInstalled) {
    return;
  }

  std::lock_guard lock{state.mutex};

  struct sigaction action {};
  action.sa_sigaction = &signalHandler;
  action.sa_flags = SA_SIGINFO;
  sigaction(SIGFPE, &action, nullptr);

  state.isSignalHandlerInstalled = true;
}

void FpeMonitor::disable() {
#if defined(__linux__) && defined(__x86_64__)
  std::feclearexcept(m_excepts);
  assert(!stack().empty() && "FPE stack shouldn't be empty at this point");
  stack().pop();
  // disable excepts we enabled here
  fedisableexcept(m_excepts);
  if (!stack().empty()) {
    // restore excepts from next stack element
    std::feclearexcept(stack().top()->m_excepts);
    feenableexcept(stack().top()->m_excepts);
  }
#endif
}

std::stack<FpeMonitor *> &FpeMonitor::stack() {
  static thread_local std::stack<FpeMonitor *> monitors;
  return monitors;
}

FpeMonitor::GlobalState &FpeMonitor::globalState() {
  static GlobalState state{};
  return state;
}

std::ostream &operator<<(std::ostream &os, FpeType type) {
#define CASE(x)    \
  case FpeType::x: \
    os << #x;      \
    break;

  switch (type) {
    CASE(INTDIV)
    CASE(INTOVF)
    CASE(FLTDIV)
    CASE(FLTOVF)
    CASE(FLTUND)
    CASE(FLTRES)
    CASE(FLTINV)
    CASE(FLTSUB)
  }
#undef CASE

  return os;
}

std::string FpeMonitor::stackTraceToString(
    const boost::stacktrace::stacktrace &st, std::size_t depth) {
  return boost::stacktrace::detail::to_string(st.as_vector().data(),
                                              std::min(depth, st.size()));
}

std::string FpeMonitor::getSourceLocation(
    const boost::stacktrace::frame &frame) {
  return frame.source_file() + ":" + std::to_string(frame.source_line());
}

bool FpeMonitor::canSymbolize() {
#if defined(BOOST_STACKTRACE_USE_NOOP)
  return false;
#else
  return true;
#endif
}

}  // namespace Acts
