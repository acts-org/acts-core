// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Concepts.hpp"

// STL include(s)
#include <cassert>
#include <ctime>
#include <exception>
#include <functional>
#include <iomanip>
#include <iostream>
#include <memory>
#include <optional>
#include <sstream>
#include <stdexcept>
#include <string>
#include <string_view>
#include <thread>
#include <utility>

#include <nlohmann/json.hpp>

/// @defgroup Logging Logging

// clang-format off
/// @brief macro to use a local Acts::Logger object
/// @ingroup Logging
///
/// @param log_object logger instance of type
//         <tt>std::unique_ptr<const Acts::Logger></tt>
///
/// @pre In the current scope, the symbol @c logger is not yet defined.
/// @post The ownership of the given @c log_object is transferred and
///       @c log_object should not be used directly any more.
///
/// This macro allows to use a locally defined logging object with the ACTS_*
/// logging macros. The envisaged usage is the following:
///
/// @code{.cpp}
/// void myFunction() {
///    std::unique_ptr<const Acts::Logger> myLogger
///        = /* .. your initialization .. */;
///    ACTS_LOCAL_LOGGER(std::move(myLogger));
///
///    ACTS_VERBOSE("hello world!");
/// }
/// @endcode
#define ACTS_LOCAL_LOGGER(log_object)                                          \
  struct __local_acts_logger                                                   \
  {                                                                            \
    __local_acts_logger(std::unique_ptr<const ::Acts::Logger> logger):         \
      m_logger(std::move(logger))                                              \
    {}                                                                         \
                                                                               \
    const ::Acts::Logger& operator()() const                                   \
    {                                                                          \
      return *m_logger;                                                        \
    }                                                                          \
                                                                               \
    std::unique_ptr<const ::Acts::Logger> m_logger;                            \
  };                                                                           \
  __local_acts_logger logger(log_object);

// Debug level agnostic implementation of the ACTS_XYZ logging macros
#define ACTS_LOG(level, x, ...)                                                     \
  do {                                                                         \
    if (logger().doPrint(level)) {                                             \
  using namespace Acts::Logging::Literals; \
      std::ostringstream os;                                                   \
      os << x;                                                                 \
      logger().log(level, os.str() __VA_OPT__(,) __VA_ARGS__);                                           \
    }                                                                          \
  }                                                                            \
  while(0)

/// @brief macro for verbose debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::VERBOSE.
#define ACTS_VERBOSE(...)  ACTS_LOG(Acts::Logging::VERBOSE, __VA_ARGS__)

/// @brief macro for debug debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::DEBUG.
#define ACTS_DEBUG(...)  ACTS_LOG(Acts::Logging::DEBUG, __VA_ARGS__)

/// @brief macro for info debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::INFO.
#define ACTS_INFO(...)  ACTS_LOG(Acts::Logging::INFO, __VA_ARGS__)

/// @brief macro for warning debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::WARNING.
#define ACTS_WARNING( ...)  ACTS_LOG(Acts::Logging::WARNING, __VA_ARGS__)

/// @brief macro for error debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::ERROR.
#define ACTS_ERROR( ...)  ACTS_LOG(Acts::Logging::ERROR,  __VA_ARGS__)

/// @brief macro for fatal debug output
/// @ingroup Logging
///
/// @param x debug message
///
/// @pre @c logger() must be a valid expression in the scope where this
///      macro is used and it must return a Acts::Logger object.
///
/// The debug message is printed if the current Acts::Logging::Level <=
/// Acts::Logging::FATAL.
#define ACTS_FATAL(...)  ACTS_LOG(Acts::Logging::FATAL, __VA_ARGS__)
// clang-format on

namespace Acts {

/// @brief debug output related helper classes and functions
/// @ingroup Logging
namespace Logging {
/// @brief constants steering the debug output
///
/// All messages with a debug level equal or higher than the currently set
/// debug output level will be printed.
enum Level {
  VERBOSE = 0,  ///< VERBOSE level
  DEBUG,        ///< DEBUG level
  INFO,         ///< INFO level
  WARNING,      ///< WARNING level
  ERROR,        ///< ERROR level
  FATAL,        ///< FATAL level
  MAX           ///< Must be kept above the maximum supported debug level
};

inline std::string_view levelName(Level level) {
  switch (level) {
    case Level::VERBOSE:
      return "VERBOSE";
    case Level::DEBUG:
      return "DEBUG";
    case Level::INFO:
      return "INFO";
    case Level::WARNING:
      return "WARNING";
    case Level::ERROR:
      return "ERROR";
    case Level::FATAL:
      return "FATAL";
    case Level::MAX:
      return "MAX";
    default:
      throw std::invalid_argument{"Unknown level"};
  }
}

#ifdef DOXYGEN
/// @brief Get debug level above which an exception will be thrown after logging
///
/// All messages with a debug level equal or higher than the return value of
/// this function will cause an exception to be thrown after log emission.
///
/// @note Depending on preprocessor settings @c ACTS_ENABLE_LOG_FAILURE_THRESHOLD
///       and @c ACTS_LOG_FAILURE_THRESHOLD, this operations is either constexpr
///       or a runtime operation.
Level getFailureThreshold();

#else

#ifdef ACTS_ENABLE_LOG_FAILURE_THRESHOLD
#ifdef ACTS_LOG_FAILURE_THRESHOLD
// We have a fixed compile time log failure threshold
constexpr Level getFailureThreshold() {
  return Level::ACTS_LOG_FAILURE_THRESHOLD;
}
#else
Level getFailureThreshold();
#endif
#else
constexpr Level getFailureThreshold() {
  // Default "NO" failure threshold
  return Level::MAX;
}
#endif

#endif

/// @brief Set debug level above which an exception will be thrown after logging
///
/// All messages with a debug level equal or higher than @p level will
/// cause an exception to be thrown after log emission.
///
/// @warning The runtime log failure threshold is **global state**, therefore
///          this function is  **not threadsafe**. The intention is that this
///          level is set once, before multi-threaded execution begins, and then
///          not modified before the end of the job.
/// @note This function is only available if @c ACTS_LOG_FAILURE_THRESHOLD is
///       unset, i.e. no compile-time threshold is used. Otherwise an
///       exception is thrown.
void setFailureThreshold(Level level);

/// Custom exception class so threshold failures can be caught
class ThresholdFailure : public std::runtime_error {
  using std::runtime_error::runtime_error;
};

/// Helper class that changes the failure threshold for the duration of its
/// lifetime.
class ScopedFailureThreshold {
 public:
  explicit ScopedFailureThreshold(Level level) { setFailureThreshold(level); }
  ScopedFailureThreshold(const ScopedFailureThreshold&) = delete;
  ScopedFailureThreshold& operator=(const ScopedFailureThreshold&) = delete;
  ScopedFailureThreshold(ScopedFailureThreshold&&) = delete;
  ScopedFailureThreshold& operator=(ScopedFailureThreshold&&) = delete;

  ~ScopedFailureThreshold() noexcept;

 private:
  Level m_previousLevel{getFailureThreshold()};
};

/// @brief abstract base class for printing debug output
///
/// Implementations of this interface need to define how and where to @a print
/// debug messages (e.g. to a file, to a stream into a database etc).
class OutputPrintPolicy {
 public:
  /// virtual default destructor
  virtual ~OutputPrintPolicy() = default;

  /// @brief handle output of debug message
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] input text of debug message
  virtual void flush(const Level& lvl, const std::string& input) = 0;

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  virtual void flush(const Level& lvl, nlohmann::json&& json) = 0;

  /// Return the name of the print policy
  /// @return the name
  virtual const std::string& name() const = 0;

  /// Make a copy of this print policy with a new name
  /// @param name the new name
  /// @return the copy
  virtual std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& name) const = 0;
};

/// @brief abstract base class for filtering debug output
///
/// Implementations of this interface need to define whether a debug message
/// with a certain debug level is processed or filtered out.
class OutputFilterPolicy {
 public:
  /// virtual default destructor
  virtual ~OutputFilterPolicy() = default;

  /// @brief decide whether a debug message should be processed
  ///
  /// @param [in] lvl debug level of debug message
  ///
  /// @return @c true of debug message should be processed, @c false if debug
  ///         message should be skipped
  virtual bool doPrint(const Level& lvl) const = 0;

  /// Get the level of this filter policy
  /// @return the levele
  virtual Level level() const = 0;

  /// Make a copy of this filter policy with a new level
  /// @param level the new level
  /// @return the new copy
  virtual std::unique_ptr<OutputFilterPolicy> clone(Level level) const = 0;
};

/// @brief default filter policy for debug messages
///
/// All debug messages with a debug level equal or larger to the specified
/// threshold level are processed.
class DefaultFilterPolicy final : public OutputFilterPolicy {
 public:
  /// @brief constructor
  ///
  /// @param [in] lvl threshold debug level
  explicit DefaultFilterPolicy(Level lvl) : m_level(lvl) {
    if (lvl > getFailureThreshold()) {
      throw ThresholdFailure(
          "Requested debug level is incompatible with "
          "the ACTS_LOG_FAILURE_THRESHOLD=" +
          std::string{levelName(getFailureThreshold())} +
          " configuration. See "
          "https://acts.readthedocs.io/en/latest/core/misc/"
          "logging.html#logging-thresholds");
    }
  }

  /// virtual default destructor
  ~DefaultFilterPolicy() override = default;

  /// @brief decide whether a debug message should be processed
  ///
  /// @param [in] lvl debug level of debug message
  ///
  /// @return @c true if @p lvl >= #m_level, otherwise @c false
  bool doPrint(const Level& lvl) const override { return m_level <= lvl; }

  /// Get the level of this filter policy
  /// @return the levele
  Level level() const override { return m_level; }

  /// Make a copy of this filter policy with a new level
  /// @param level the new level
  /// @return the new copy
  std::unique_ptr<OutputFilterPolicy> clone(Level level) const override {
    return std::make_unique<DefaultFilterPolicy>(level);
  }

 private:
  /// threshold debug level for messages to be processed
  Level m_level;
};

/// @brief base class for decorating the debug output
///
/// Derived classes may augment the debug message with additional information.
/// Chaining different decorators is possible to customize the output to your
/// needs.
class OutputDecorator : public OutputPrintPolicy {
 public:
  /// @brief constructor wrapping actual output print policy
  ///
  /// @param [in] wrappee output print policy object which is wrapped by this
  ///        decorator object
  explicit OutputDecorator(std::unique_ptr<OutputPrintPolicy> wrappee)
      : m_wrappee(std::move(wrappee)) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  ///
  /// This function delegates the flushing of the debug message to its wrapped
  /// object.
  void flush(const Level& lvl, const std::string& input) override {
    m_wrappee->flush(lvl, input);
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    m_wrappee->flush(lvl, std::move(json));
  }

  /// Return the name of the output decorator (forwards to wrappee)
  /// @return the name
  const std::string& name() const override { return m_wrappee->name(); }

 protected:
  /// wrapped object for printing the debug message
  std::unique_ptr<OutputPrintPolicy> m_wrappee;
};

/// @brief decorate debug message with a name
///
/// The debug message is complemented with a name.
class NamedOutputDecorator final : public OutputDecorator {
 public:
  /// @brief constructor
  ///
  /// @param [in] wrappee  output print policy object to be wrapped
  /// @param [in] name     name to be added to debug message
  /// @param [in] maxWidth maximum width of field used for name
  NamedOutputDecorator(std::unique_ptr<OutputPrintPolicy> wrappee,
                       const std::string& name, unsigned int maxWidth = 15)
      : OutputDecorator(std::move(wrappee)),
        m_name(name),
        m_maxWidth(maxWidth) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  ///
  /// This function prepends the given name to the debug message and then
  /// delegates the flushing of the whole message to its wrapped object.
  void flush(const Level& lvl, const std::string& input) override {
    std::ostringstream os;
    os << std::left << std::setw(m_maxWidth) << m_name.substr(0, m_maxWidth - 3)
       << input;
    OutputDecorator::flush(lvl, os.str());
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    json["name"] = m_name;
    OutputDecorator::flush(lvl, std::move(json));
  }

  /// Make a copy of this print policy with a new name
  /// @param name the new name
  /// @return the copy
  std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& name) const override {
    return std::make_unique<NamedOutputDecorator>(m_wrappee->clone(name), name,
                                                  m_maxWidth);
  }

  /// Get this named output decorators name
  /// @return the name
  const std::string& name() const override { return m_name; }

 private:
  /// name to be prepended
  std::string m_name;

  /// maximum width of field for printing the name
  unsigned int m_maxWidth;
};

/// @brief decorate debug message with a time stamp
///
/// The debug message is complemented with a time stamp.
class TimedOutputDecorator final : public OutputDecorator {
 public:
  /// @brief constructor
  ///
  /// @param [in] wrappee output print policy object to be wrapped
  /// @param [in] format  format of time stamp (see std::strftime)
  TimedOutputDecorator(std::unique_ptr<OutputPrintPolicy> wrappee,
                       const std::string& format = "%X")
      : OutputDecorator(std::move(wrappee)), m_format(format) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  ///
  /// This function prepends a time stamp to the debug message and then
  /// delegates the flushing of the whole message to its wrapped object.
  void flush(const Level& lvl, const std::string& input) override {
    std::ostringstream os;
    os << std::left << std::setw(12) << now() << input;
    OutputDecorator::flush(lvl, os.str());
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    json["time"] = now();
    OutputDecorator::flush(lvl, std::move(json));
  }

  /// Make a copy of this print policy with a new name
  /// @param name the new name
  /// @return the copy
  std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& name) const override {
    return std::make_unique<TimedOutputDecorator>(m_wrappee->clone(name),
                                                  m_format);
  }

 private:
  /// @brief get current time stamp
  ///
  /// @return current time stamp as string
  std::string now() const {
    char buffer[20];
    time_t t{};
    std::time(&t);
    struct tm tbuf {};
    std::strftime(buffer, sizeof(buffer), m_format.c_str(),
                  localtime_r(&t, &tbuf));
    return buffer;
  }

  /// format of the time stamp (see std::strftime for details)
  std::string m_format;
};

/// @brief decorate debug message with a thread ID
///
/// The debug message is complemented with a thread ID.
class ThreadOutputDecorator final : public OutputDecorator {
 public:
  /// @brief constructor
  ///
  /// @param [in] wrappee output print policy object to be wrapped
  explicit ThreadOutputDecorator(std::unique_ptr<OutputPrintPolicy> wrappee)
      : OutputDecorator(std::move(wrappee)) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  ///
  /// This function prepends the thread ID to the debug message and then
  /// delegates the flushing of the whole message to its wrapped object.
  void flush(const Level& lvl, const std::string& input) override {
    std::ostringstream os;
    os << std::left << std::setw(20) << std::this_thread::get_id() << input;
    OutputDecorator::flush(lvl, os.str());
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    std::ostringstream os;
    os << std::this_thread::get_id();
    json["thread"] = os.str();
    OutputDecorator::flush(lvl, std::move(json));
  }

  /// Make a copy of this print policy with a new name
  /// @param name the new name
  /// @return the copy
  std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& name) const override {
    return std::make_unique<ThreadOutputDecorator>(m_wrappee->clone(name));
  }
};

/// @brief decorate debug message with its debug level
///
/// The debug message is complemented with its debug level.
class LevelOutputDecorator final : public OutputDecorator {
 public:
  /// @brief constructor
  ///
  /// @param [in] wrappee output print policy object to be wrapped
  explicit LevelOutputDecorator(std::unique_ptr<OutputPrintPolicy> wrappee)
      : OutputDecorator(std::move(wrappee)) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  ///
  /// This function prepends the debug level to the debug message and then
  /// delegates the flushing of the whole message to its wrapped object.
  void flush(const Level& lvl, const std::string& input) override {
    std::ostringstream os;
    os << std::left << std::setw(10) << toString(lvl) << input;
    OutputDecorator::flush(lvl, os.str());
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    json["level"] = toString(lvl);
    OutputDecorator::flush(lvl, std::move(json));
  }

  /// Make a copy of this print policy with a new name
  /// @param name the new name
  /// @return the copy
  std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& name) const override {
    return std::make_unique<LevelOutputDecorator>(m_wrappee->clone(name));
  }

 private:
  /// @brief convert debug level to string
  ///
  /// @param [in] lvl debug level
  ///
  /// @return string representation of debug level
  std::string toString(const Level& lvl) const {
    static const char* const buffer[] = {"VERBOSE", "DEBUG", "INFO",
                                         "WARNING", "ERROR", "FATAL"};
    return buffer[lvl];
  }
};

/// @brief default print policy for debug messages
///
/// This class allows to print debug messages without further modifications to
/// a specified output stream.
class DefaultPrintPolicy final : public OutputPrintPolicy {
 public:
  /// @brief constructor
  ///
  /// @param [in] out pointer to output stream object
  ///
  /// @pre @p out is non-zero
  explicit DefaultPrintPolicy(std::ostream* out = &std::cout) : m_out(out) {}

  /// @brief flush the debug message to the destination stream
  ///
  /// @param [in] lvl   debug level of debug message
  /// @param [in] input text of debug message
  void flush(const Level& lvl, const std::string& input) final {
    (*m_out) << input << std::endl;
    if (lvl >= getFailureThreshold()) {
      throw ThresholdFailure(
          "Previous debug message exceeds the "
          "ACTS_LOG_FAILURE_THRESHOLD=" +
          std::string{levelName(getFailureThreshold())} +
          " configuration, bailing out. See "
          "https://acts.readthedocs.io/en/latest/core/misc/"
          "logging.html#logging-thresholds");
    }
  }

  /// @brief handle output as json
  ///
  /// @param [in] lvl   debug output level of message
  /// @param [in] json json object representing the log record
  void flush(const Level& lvl, nlohmann::json&& json) override {
    flush(lvl, json.dump());
  }

  /// Fulfill @c OutputPrintPolicy interface. This policy doesn't actually have a
  /// name, so the assumption is that somewhere in the decorator hierarchy,
  /// there is something that returns a name without delegating to a wrappee,
  /// before reaching this overload.
  /// @note This method will throw an exception
  /// @return the name, but it never returns
  const std::string& name() const override {
    throw std::runtime_error{
        "Default print policy doesn't have a name. Is there no named output in "
        "the decorator chain?"};
  };

  /// Make a copy of this print policy with a new name
  /// @return the copy
  std::unique_ptr<OutputPrintPolicy> clone(
      const std::string& /*name*/) const override {
    return std::make_unique<DefaultPrintPolicy>(m_out);
  };

 private:
  /// pointer to destination output stream
  std::ostream* m_out;
};

namespace detail {

template <typename T>
concept JsonConvertible = requires(T t, nlohmann::json j) {
  { j = t };
};

template <JsonConvertible T>
class structured_log_key_v;
class structured_log_key {
 public:
  explicit structured_log_key(std::string_view key) : m_key{key} {
    if (m_key.empty() || m_key == "message") {
      throw std::runtime_error{"Invalid structured logging key '" +
                               std::string{key} + "'"};
    }
  }

  structured_log_key(const structured_log_key&) = delete;
  structured_log_key(structured_log_key&&) = delete;
  structured_log_key& operator=(const structured_log_key&) = delete;
  structured_log_key& operator=(structured_log_key&&) = delete;

  template <JsonConvertible T>
  structured_log_key_v<T> operator=(T&& value) {
    return structured_log_key_v<T>{m_key, std::forward<T>(value)};
  }

  std::string_view key() const { return m_key; }

 protected:
  std::string_view m_key;
};

template <JsonConvertible T>
class structured_log_key_v : public structured_log_key {
 public:
  explicit structured_log_key_v(std::string_view key, const T& value)
      : structured_log_key{key}, m_value{value} {}

  structured_log_key_v(const structured_log_key_v&) = delete;
  structured_log_key_v(structured_log_key_v&&) = delete;
  structured_log_key_v& operator=(const structured_log_key_v&) = delete;
  structured_log_key_v& operator=(structured_log_key_v&&) = delete;

  const T& value() && { return m_value; }

 private:
  const T& m_value;
};

}  // namespace detail

namespace Literals {
/// This literal supports structured logging with key value pairs in the logging
/// macros.
/// @param key the key of the value
/// @param len the length of the key
/// @return a structured log key
inline detail::structured_log_key operator""_key(const char* key,
                                                 std::size_t len) {
  return detail::structured_log_key{std::string_view{key, len}};
}
}  // namespace Literals

}  // namespace Logging

/// @brief class for printing debug output
///
/// This class provides the user interface for printing debug messages with
/// different levels of severity.
///
/// @ingroup Logging
class Logger {
 public:
  /// Controls the output mode of this logger.
  enum class OutputMode {
    /// This mode outputs log messages like:
    /// ```
    /// LoggerName LEVEL Message key1=value key2=value
    /// ```
    Default,

    /// This mode outputs log messages like:
    /// ```
    /// LoggerName LEVEL Message
    /// ```
    ///
    /// It discards all structured logging info
    Plain,

    /// This mode outputs the log entry as a JSON string
    /// ```json
    /// {"name":"LoggerName","level":"LEVEL","message":"Message","key1":"value","key2":"value"}
    /// ```
    Structured
  };

  /// @brief construct from output print and filter policy
  ///
  /// @param [in] pPrint  policy for printing debug messages
  /// @param [in] pFilter policy for filtering debug messages
  /// @param [in] mode The output mode for this logger
  Logger(std::unique_ptr<Logging::OutputPrintPolicy> pPrint,
         std::unique_ptr<Logging::OutputFilterPolicy> pFilter,
         OutputMode mode = OutputMode::Default)
      : m_printPolicy(std::move(pPrint)),
        m_filterPolicy(std::move(pFilter)),
        m_mode{mode} {}

  /// Set the output mode for this logger
  /// @param mode
  void setOutputMode(OutputMode mode) { m_mode = mode; }

  /// Get the output mode for this logger
  /// @return the output mode
  OutputMode outputMode() { return m_mode; }

  /// @brief decide whether a message with a given debug level has to be printed
  ///
  /// @param [in] lvl debug level of debug message
  ///
  /// @return @c true if debug message should be printed, otherwise @c false
  bool doPrint(const Logging::Level& lvl) const {
    return m_filterPolicy->doPrint(lvl);
  }

  /// @brief log a debug message
  ///
  /// @param [in] lvl debug level of debug message
  /// @param [in] input text of debug message
  void log(const Logging::Level& lvl, const std::string& input) const {
    if (doPrint(lvl)) {
      m_printPolicy->flush(lvl, input);
    }
  }

  /// @brief log a debug message with structured output
  ///
  /// @param [in] lvl debug level of debug message
  /// @param [in] input text of debug message
  /// @param [in] args key value pairs to be added to the log message, see @ref
  ///             Acts::Logging::Literals::_key
  template <typename... Args>
  void log(Logging::Level lvl, const std::string& message,
           Logging::detail::structured_log_key_v<Args>&&... args) const {
    if (!doPrint(lvl)) {
      return;
    }

    auto makeJson = [&]() {
      nlohmann::json j;
      auto add_to_json = [&j](auto&& arg) {
        j[std::string{arg.key()}] = std::forward<decltype(arg)>(arg).value();
      };
      {
        (add_to_json(
             std::forward<Logging::detail::structured_log_key_v<Args>>(args)),
         ...);
      }
      return j;
    };

    switch (m_mode) {
      using enum OutputMode;

      case Plain:
        m_printPolicy->flush(lvl, message);
        break;
      case Default: {
        std::stringstream os;
        os << message;
        nlohmann::json j = makeJson();
        for (const auto& [key, value] : j.items()) {
          os << " " << key << "=" << value;
        }
        m_printPolicy->flush(lvl, os.str());
        break;
      }
      case Structured: {
        nlohmann::json j = makeJson();
        j["message"] = message;
        m_printPolicy->flush(lvl, std::move(j));
        break;
      }
    }
  }

  /// Return the print policy for this logger
  /// @return the print policy
  const Logging::OutputPrintPolicy& printPolicy() const {
    return *m_printPolicy;
  }

  /// Return the filter policy for this logger
  /// @return the filter policy
  const Logging::OutputFilterPolicy& filterPolicy() const {
    return *m_filterPolicy;
  }

  /// Return the level of the filter policy of this logger
  /// @return the level
  Logging::Level level() const { return m_filterPolicy->level(); }

  /// Return the name of the print policy of this logger
  /// @return the name
  const std::string& name() const { return m_printPolicy->name(); }

  /// Make a copy of this logger, optionally changing the name or the level
  /// @param _name the optional new name
  /// @param _level the optional new level
  std::unique_ptr<Logger> clone(
      const std::optional<std::string>& _name = std::nullopt,
      const std::optional<Logging::Level>& _level = std::nullopt) const {
    return std::make_unique<Logger>(
        m_printPolicy->clone(_name.value_or(name())),
        m_filterPolicy->clone(_level.value_or(level())));
  }

  /// Make a copy of the logger, with a new level. Convenience function for
  /// if you only want to change the level but not the name.
  /// @param _level the new level
  /// @return the new logger
  std::unique_ptr<Logger> clone(Logging::Level _level) const {
    return clone(std::nullopt, _level);
  }

  /// Make a copy of the logger, with a suffix added to the end of it's
  /// name. You can also optionally supply a new level
  /// @param suffix the suffix to add to the end of the name
  /// @param _level the optional new level
  std::unique_ptr<Logger> cloneWithSuffix(
      const std::string& suffix,
      std::optional<Logging::Level> _level = std::nullopt) const {
    return clone(name() + suffix, _level.value_or(level()));
  }

  /// Helper function so a logger reference can be used as is with the logging
  /// macros
  const Logger& operator()() const { return *this; }

 private:
  /// policy object for printing debug messages
  std::unique_ptr<Logging::OutputPrintPolicy> m_printPolicy;

  /// policy object for filtering debug messages
  std::unique_ptr<Logging::OutputFilterPolicy> m_filterPolicy;

  /// the stored output mode
  OutputMode m_mode{OutputMode::Default};
};

/// @brief get default debug output logger
///
/// @param [in] name       name of the logger instance
/// @param [in] lvl        debug threshold level
/// @param [in] log_stream output stream used for printing debug messages
///
/// This function returns a pointer to a Logger instance with the following
/// decorations enabled:
/// - time stamps
/// - name of logging instance
/// - debug level
///
/// @return pointer to logging instance
std::unique_ptr<const Logger> getDefaultLogger(
    const std::string& name, const Logging::Level& lvl,
    std::ostream* log_stream = &std::cout);

const Logger& getDummyLogger();

}  // namespace Acts
