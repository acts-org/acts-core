// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "ActsExamples/Framework/DataHandle.hpp"
#include "ActsExamples/Framework/SequenceElement.hpp"
#include "ActsExamples/Framework/WhiteBoard.hpp"

#include <tuple>

namespace Acts::Test {

struct DummySequenceElement : public ActsExamples::SequenceElement {
  ActsExamples::ProcessCode initialize() override { return {}; };
  ActsExamples::ProcessCode finalize() override { return {}; };
  ActsExamples::ProcessCode internalExecute(
      const ActsExamples::AlgorithmContext & /*context*/) override {
    return {};
  };
  std::string name() const override { return {}; };
};

template <typename T>
void addToWhiteBoard(std::string name, T data, ActsExamples::WhiteBoard &wb) {
  DummySequenceElement dummyElement;

  ActsExamples::WriteDataHandle<T> handle(&dummyElement, name + "handle");
  handle.initialize(name);
  handle(wb, std::move(data));
}

template <typename T>
T getFromWhiteBoard(std::string name, ActsExamples::WhiteBoard &wb) {
  DummySequenceElement dummyElement;

  ActsExamples::ReadDataHandle<T> handle(&dummyElement, name + "handle");
  handle.initialize(name);
  return handle(wb);
}

template <typename val_tuple_t = std::tuple<>,
          typename str_tuple_t = std::tuple<>>
struct GenericReadWriteTool {
  val_tuple_t tuple;
  str_tuple_t strings;

  constexpr static std::size_t kSize = std::tuple_size_v<val_tuple_t>;
  static_assert(kSize == std::tuple_size_v<str_tuple_t>);

  template <typename T>
  auto add(const std::string &name, T value) {
    auto newTuple = std::tuple_cat(tuple, std::tuple<T>{value});
    auto newStrings = std::tuple_cat(strings, std::tuple<std::string>{name});

    GenericReadWriteTool<decltype(newTuple), decltype(newStrings)> newInstance;
    newInstance.tuple = std::move(newTuple);
    newInstance.strings = std::move(newStrings);

    return newInstance;
  }

  template <typename writer_t>
  auto write(writer_t &writer, std::size_t eventId = 0) {
    ActsExamples::WhiteBoard board;
    ActsExamples::AlgorithmContext ctx(0, eventId, board);

    auto add = [&](auto &self, auto N) {
      if constexpr (N() < kSize) {
        addToWhiteBoard(std::get<N()>(strings), std::get<N()>(tuple), board);
        self(self, std::integral_constant<std::size_t, N() + 1>{});
      }
    };

    add(add, std::integral_constant<std::size_t, 0>{});

    writer.internalExecute(ctx);
  }

  template <typename reader_t>
  auto read(reader_t &reader, std::size_t eventId = 0) {
    ActsExamples::WhiteBoard board;
    ActsExamples::AlgorithmContext ctx(0, eventId, board);

    reader.internalExecute(ctx);

    auto get = [&](auto &self, auto res, auto N) {
      if constexpr (N() < kSize) {
        using T = std::tuple_element_t<N(), val_tuple_t>;
        auto val = getFromWhiteBoard<T>(std::get<N()>(strings), board);
        return self(self, std::tuple_cat(res, std::make_tuple(val)),
                    std::integral_constant<std::size_t, N() + 1>{});
      } else {
        return res;
      }
    };

    return get(get, std::tuple<>{}, std::integral_constant<std::size_t, 0>{});
  }
};

}  // namespace Acts::Test
