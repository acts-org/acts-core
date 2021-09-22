// This file is part of the Acts project.
//
// Copyright (C) 2020-2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

// System include(s)
#include <string>

// Acts include(s)
#include "Acts/Utilities/Logger.hpp"

// SYCL plugin include(s)
#include "Acts/Plugins/Sycl/Utilities/DeviceSelector.hpp"
#include "Acts/Plugins/Sycl/Utilities/QueueWrapper.hpp"

// SYCL include
#include <CL/sycl.hpp>

namespace Acts::Sycl {

QueueWrapper::QueueWrapper(const std::string& deviceNameSubstring,
                           std::unique_ptr<const Logger> incomingLogger)
    : m_queue(nullptr), m_ownsQueue(true), m_logger(std::move(incomingLogger)) {
  // SYCL kernel exceptions are asynchronous
  auto exception_handler = [&log =
                                m_logger](cl::sycl::exception_list exceptions) {
    for (std::exception_ptr const& e : exceptions) {
      try {
        std::rethrow_exception(e);
      } catch (std::exception& e) {
        LoggerWrapper logger(*log);
        ACTS_FATAL("Caught asynchronous (kernel) SYCL exception:\n" << e.what())
      }
    }
  };

  // Create queue with custom device selector
  m_queue = new cl::sycl::queue(DeviceSelector(deviceNameSubstring),
                                exception_handler);
  m_ownsQueue = true;

  // See which device we are running on.
  LoggerWrapper logger(*m_logger);
  ACTS_INFO("Running on: "
            << m_queue->get_device().get_info<cl::sycl::info::device::name>());
}

QueueWrapper::QueueWrapper(cl::sycl::queue& queue,
                           std::unique_ptr<const Logger> incomingLogger)
    : m_queue(&queue), m_ownsQueue(false),
      m_logger(std::move(incomingLogger)) {}

QueueWrapper::QueueWrapper(QueueWrapper&& parent) noexcept
    : m_queue(parent.m_queue),
      m_ownsQueue(parent.m_ownsQueue),
      m_logger(std::move(parent.m_logger)) {
  parent.m_queue = nullptr;
  parent.m_ownsQueue = false;
}

QueueWrapper::QueueWrapper(const QueueWrapper& other)
    : m_queue(other.m_queue),
      m_ownsQueue(false),
      m_logger(getDefaultLogger("Sycl::QueueWrapper", Logging::INFO)) {}

QueueWrapper::~QueueWrapper() {
  if (m_ownsQueue && (m_queue != nullptr)) {
    delete m_queue;
  }
}

QueueWrapper& QueueWrapper::operator=(QueueWrapper&& rhs) noexcept {
  // Check whether we have to do anything
  if (this == &rhs) {
    return *this;
  }

  // Destroy this queue,
  if (m_ownsQueue && (m_queue != nullptr)) {
    delete m_queue;
  }

  // Perform the move
  m_queue = rhs.m_queue;
  m_ownsQueue = rhs.m_ownsQueue;
  m_logger = std::move(rhs.m_logger);
  rhs.m_queue = nullptr;
  rhs.m_ownsQueue = false;

  // Return this object.
  return *this;
}

QueueWrapper& QueueWrapper::operator=(const QueueWrapper& other) {
  // Check whether we have to do anything
  if (this == &other) {
    return *this;
  }

  m_queue = other.m_queue;
  m_ownsQueue = false;
  return *this;
}

const cl::sycl::queue* QueueWrapper::getQueue() const {
  return m_queue;
}

cl::sycl::queue* QueueWrapper::getQueue() {
  return m_queue;
}

const cl::sycl::queue* QueueWrapper::operator->() const {
  return m_queue;
}

cl::sycl::queue* QueueWrapper::operator->() {
  return m_queue;
}

const cl::sycl::queue& QueueWrapper::operator*() const {
  return *m_queue;
}

cl::sycl::queue& QueueWrapper::operator*() {
  return *m_queue;
}

}  // namespace Acts::Sycl
