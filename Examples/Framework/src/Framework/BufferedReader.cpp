// SPDX-PackageName: "ACTS"
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MPL-2.0

#include "ActsExamples/Framework/BufferedReader.hpp"

#include "Acts/Utilities/Logger.hpp"
#include "ActsExamples/Framework/AlgorithmContext.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <random>
#include <utility>

namespace ActsExamples {

BufferedReader::BufferedReader(const Config &config, Acts::Logging::Level level)
    : m_cfg(config), m_logger(Acts::getDefaultLogger(name(), level)) {
  if (!m_cfg.upstreamReader) {
    throw std::invalid_argument("No upstream reader provided!");
  }

  // Register write and read handles of the upstream reader
  for (auto rh : m_cfg.upstreamReader->readHandles()) {
    registerReadHandle(*rh);
  }

  for (auto wh : m_cfg.upstreamReader->writeHandles()) {
    registerWriteHandle(*wh);
  }

  // Read the events
  auto [ebegin, eend] = m_cfg.upstreamReader->availableEvents();
  if (eend - ebegin < m_cfg.bufferSize) {
    throw std::runtime_error("Reader does not provide enough events");
  }

  ACTS_INFO("Start reading events into buffer...");

  m_buffer.reserve(eend - ebegin);
  for (auto i = ebegin; i < ebegin + m_cfg.bufferSize; ++i) {
    auto board = std::make_unique<ActsExamples::WhiteBoard>(m_logger->clone());
    ActsExamples::AlgorithmContext ctx(0, i, *board);

    ACTS_DEBUG("Read event " << i << " into buffer");
    m_cfg.upstreamReader->read(ctx);
    m_buffer.emplace_back(std::move(board));
  }

  ACTS_INFO("Filled " << m_buffer.size() << " events into the buffer");
}

ProcessCode BufferedReader::read(const AlgorithmContext &ctx) {
  // Set up a random event selection that is consistent if multiple
  // BufferedReader are used within a workflow The linear congruential engine is
  // chosen since it is cheap to instantiate. For each eventNumber, it is put in
  // a reproducible state.
  std::minstd_rand rng(m_cfg.selectionSeed);
  rng.discard(ctx.eventNumber);

  /// Sample from the buffer and transfer the content
  std::uniform_int_distribution<std::size_t> dist(0, m_cfg.bufferSize - 1);

  const auto entry = dist(rng);
  ctx.eventStore.copyFrom(*m_buffer.at(entry));

  ACTS_DEBUG("Use buffer entry " << entry << " for event " << ctx.eventNumber);

  return ProcessCode::SUCCESS;
}

}  // namespace ActsExamples
