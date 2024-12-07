// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#include <boost/test/unit_test.hpp>

#include "Acts/Utilities/Logger.hpp"

#include <cstddef>
#include <fstream>
#include <memory>
#include <regex>
#include <stdexcept>
#include <string>
#include <utility>
#include <vector>

#include <nlohmann/json.hpp>

using namespace Acts::Logging;

namespace Acts::Test {

/// @cond
namespace detail {
std::unique_ptr<Logger> create_logger(const std::string& logger_name,
                                      std::ostream* logfile,
                                      Logging::Level lvl) {
  auto output = std::make_unique<LevelOutputDecorator>(
      std::make_unique<NamedOutputDecorator>(
          std::make_unique<DefaultPrintPolicy>(logfile), logger_name, 30));
  auto print = std::make_unique<DefaultFilterPolicy>(lvl);
  return std::make_unique<Logger>(std::move(output), std::move(print));
}

}  // namespace detail
/// @endcond

/// @brief unit test for a certain debug level
///
/// This test checks for the expected output when using the
/// specified debug level as threshold. It also tests
/// - #ACTS_LOCAL_LOGGER
/// - Acts::getDefaultLogger
void debug_level_test(const char* output_file, Logging::Level lvl) {
  // Logs will go to this file
  std::ofstream logfile(output_file);

  // If fail-on-error is enabled, then the logger will not, and should not,
  // tolerate being set up with a coarser debug level.
  if (lvl > Logging::getFailureThreshold()) {
    BOOST_CHECK_THROW(detail::create_logger("TestLogger", &logfile, lvl),
                      std::runtime_error);
    return;
  }

  auto test = [&](std::unique_ptr<const Logger> log, const std::string& name) {
    // Set up local logger
    ACTS_LOCAL_LOGGER(std::move(log));

    // Test logging at a certain debug level
    auto test_logging = [](auto&& test_operation, Logging::Level test_lvl) {
      if (test_lvl >= Logging::getFailureThreshold()) {
        BOOST_CHECK_THROW(test_operation(), std::runtime_error);
      } else {
        test_operation();
      }
    };

    // Test logging at all debug levels
    test_logging([&] { ACTS_FATAL("fatal level"); }, FATAL);
    test_logging([&] { ACTS_ERROR("error level"); }, ERROR);
    test_logging([&] { ACTS_WARNING("warning level"); }, WARNING);
    test_logging([&] { ACTS_INFO("info level"); }, INFO);
    test_logging([&] { ACTS_DEBUG("debug level"); }, DEBUG);
    test_logging([&] { ACTS_VERBOSE("verbose level"); }, VERBOSE);
    logfile.close();

    std::string padded_name = name;
    padded_name.resize(30, ' ');

    // Compute expected output for current debug levels
    std::vector<std::string> lines{padded_name + "FATAL     fatal level",
                                   padded_name + "ERROR     error level",
                                   padded_name + "WARNING   warning level",
                                   padded_name + "INFO      info level",
                                   padded_name + "DEBUG     debug level",
                                   padded_name + "VERBOSE   verbose level"};
    lines.resize(static_cast<int>(Logging::Level::MAX) - static_cast<int>(lvl));

    // Check output
    std::ifstream infile(output_file, std::ios::in);
    std::size_t i = 0;
    for (std::string line; std::getline(infile, line); ++i) {
      BOOST_CHECK_EQUAL(line, lines.at(i));
    }
  };

  auto log = detail::create_logger("TestLogger", &logfile, lvl);
  BOOST_CHECK_EQUAL(log->name(), "TestLogger");
  auto copy = log->clone("TestLoggerClone");
  test(std::move(copy), "TestLoggerClone");
  BOOST_CHECK_EQUAL(log->name(), "TestLogger");

  auto copy2 = log->clone("TestLoggerClone");
  BOOST_CHECK_EQUAL(copy2->level(), log->level());

  auto copy3 = log->cloneWithSuffix("Suffix");
  BOOST_CHECK_EQUAL(log->level(), copy3->level());

  logfile = std::ofstream{output_file};  // clear output

  test(std::move(log), "TestLogger");
}

/// @brief unit test for FATAL debug level
BOOST_AUTO_TEST_CASE(FATAL_test) {
  debug_level_test("fatal_log.txt", FATAL);
}

/// @brief unit test for ERROR debug level
BOOST_AUTO_TEST_CASE(ERROR_test) {
  debug_level_test("error_log.txt", ERROR);
}

/// @brief unit test for WARNING debug level
BOOST_AUTO_TEST_CASE(WARNING_test) {
  debug_level_test("warning_log.txt", WARNING);
}

/// @brief unit test for INFO debug level
BOOST_AUTO_TEST_CASE(INFO_test) {
  debug_level_test("info_log.txt", INFO);
}

/// @brief unit test for DEBUG debug level
BOOST_AUTO_TEST_CASE(DEBUG_test) {
  debug_level_test("debug_log.txt", DEBUG);
}

/// @brief unit test for VERBOSE debug level
BOOST_AUTO_TEST_CASE(VERBOSE_test) {
  debug_level_test("verbose_log.txt", VERBOSE);
}

struct StructuredInfo {
  int value;
  float threshold;
  std::string name;

  StructuredInfo(int v, float t, std::string n)
      : value(v), threshold(t), name(std::move(n)) {}

  StructuredInfo(const StructuredInfo&) = delete;
  StructuredInfo& operator=(const StructuredInfo&) = delete;

  NLOHMANN_DEFINE_TYPE_INTRUSIVE(StructuredInfo, value, threshold, name);
};

BOOST_AUTO_TEST_CASE(StructuredLogTest) {
  using namespace Logging::Literals;
  using enum Logger::OutputMode;

  struct PushMode {
    PushMode(Logger& logger, Logger::OutputMode mode)
        : m_logger{&logger}, m_prev{m_logger->outputMode()} {
      m_logger->setOutputMode(mode);
    }

    ~PushMode() { m_logger->setOutputMode(m_prev); }

   private:
    Logger* m_logger;
    Logger::OutputMode m_prev;
  };

  std::stringstream sstr;
  auto _logger = detail::create_logger("TestLogger", &sstr, Logging::INFO);
  auto& logger = *_logger;

  static_assert(Logging::detail::JsonConvertible<std::string>);
  struct NotConvertible {};
  static_assert(!Logging::detail::JsonConvertible<NotConvertible>);
  static_assert(Logging::detail::JsonConvertible<StructuredInfo>);

  std::string padded_name = "TestLogger";
  padded_name.resize(30, ' ');

  {
    PushMode mode{logger, Default};
    sstr.str("");
    ACTS_DEBUG("Message", "key"_key = "value");
    BOOST_CHECK_EQUAL(sstr.str(), "");  // does not get logged at all
  }
  {
    PushMode mode{logger, Structured};
    sstr.str("");
    ACTS_DEBUG("Message", "key"_key = "value");
    BOOST_CHECK_EQUAL(sstr.str(), "");  // does not get logged at all
  }
  {
    PushMode mode{logger, Plain};
    sstr.str("");
    ACTS_DEBUG("Message", "key"_key = "value");
    BOOST_CHECK_EQUAL(sstr.str(), "");  // does not get logged at all
  }

  std::smatch m;
  const std::regex struct_expr{R"RE((\w+) +(\w+) +(.*)\s)RE"};

  {
    PushMode mode{logger, Default};
    sstr.str("");
    ACTS_WARNING("Message", "key"_key = "value");

    std::string act = sstr.str();
    std::cout << act << std::endl;
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "WARNING");
    BOOST_CHECK_EQUAL(m[3], "Message key=\"value\"");
  }

  {
    PushMode mode{logger, Structured};
    sstr.str("");
    ACTS_WARNING("Message", "key"_key = "value");
    nlohmann::json exp = {
        {"level", "WARNING"},
        {"name", "TestLogger"},
        {"message", "Message"},
        {"key", "value"},
    };
    BOOST_CHECK_EQUAL(nlohmann::json::parse(sstr.str()), exp);
  }
  {
    PushMode mode{logger, Plain};
    sstr.str("");
    ACTS_WARNING("Message", "key"_key = "value");
    std::string act = sstr.str();
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "WARNING");
    BOOST_CHECK_EQUAL(m[3], "Message");
  }

  {
    PushMode mode{logger, Default};
    sstr.str("");
    ACTS_INFO("Message " << 2, "some_key"_key = 42);

    std::string act = sstr.str();
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "INFO");
    BOOST_CHECK_EQUAL(m[3], "Message 2 some_key=42");
  }

  {
    PushMode mode{logger, Structured};
    sstr.str("");
    ACTS_INFO("Message " << 2, "some_key"_key = 42);
    nlohmann::json exp = {
        {"level", "INFO"},
        {"name", "TestLogger"},
        {"message", "Message 2"},
        {"some_key", 42},
    };
    BOOST_CHECK_EQUAL(nlohmann::json::parse(sstr.str()), exp);
  }
  {
    PushMode mode{logger, Plain};
    sstr.str("");
    ACTS_INFO("Message " << 2, "some_key"_key = 42);
    std::string act = sstr.str();
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "INFO");
    BOOST_CHECK_EQUAL(m[3], "Message 2");
  }

  {
    PushMode mode{logger, Default};
    sstr.str("");
    ACTS_INFO("Message " << 3,
              "rich_info"_key = StructuredInfo{1, 2.0, "test"});

    std::string act = sstr.str();
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "INFO");
    const std::regex ex{R"RE(.*rich_info=(.*)\s)RE"};
    BOOST_CHECK(std::regex_match(act, m, ex));
    nlohmann::json exp = {
        {"value", 1},
        {"threshold", 2.0},
        {"name", "test"},
    };
    BOOST_CHECK_EQUAL(nlohmann::json::parse(m[1].str()), exp);
  }

  {
    PushMode mode{logger, Structured};
    sstr.str("");
    ACTS_INFO("Message " << 3,
              "rich_info"_key = StructuredInfo{1, 2.0, "test"});
    nlohmann::json exp = {
        {"level", "INFO"},
        {"name", "TestLogger"},
        {"message", "Message 3"},
        {
            "rich_info",
            {
                {"value", 1},
                {"threshold", 2.0},
                {"name", "test"},
            },
        },
    };
    BOOST_CHECK_EQUAL(nlohmann::json::parse(sstr.str()), exp);
  }

  {
    PushMode mode{logger, Plain};
    sstr.str("");
    ACTS_INFO("Message " << 3,
              "rich_info"_key = StructuredInfo{1, 2.0, "test"});
    std::string act = sstr.str();
    BOOST_CHECK(std::regex_match(act, m, struct_expr));
    BOOST_CHECK_EQUAL(m[1], "TestLogger");
    BOOST_CHECK_EQUAL(m[2], "INFO");
    BOOST_CHECK_EQUAL(m[3], "Message 3");
  }
}

}  // namespace Acts::Test
