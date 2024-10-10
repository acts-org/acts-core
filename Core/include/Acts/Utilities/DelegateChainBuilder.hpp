// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/Delegate.hpp"
#include "Acts/Utilities/TypeList.hpp"

#include <tuple>
#include <type_traits>
#include <utility>

namespace Acts {

template <typename Fn, typename payload_types = TypeList<>, auto... Callables>
class DelegateChainBuilder;

template <typename R, typename... payload_types, auto... callables,
          typename... callable_args>
class DelegateChainBuilder<R(callable_args...), TypeList<payload_types...>,
                           callables...> {
  using return_type =
      std::conditional_t<std::is_same_v<R, void>, void,
                         std::array<R, sizeof...(payload_types)>>;
  using delegate_type =
      Delegate<return_type(callable_args...), void, DelegateType::Owning>;
  using tuple_type = std::tuple<payload_types...>;

 public:
  template <typename, typename Ps, auto... Cs>
  friend class DelegateChainBuilder;

  DelegateChainBuilder() = default;

  template <typename D>
  DelegateChainBuilder(const D& /*unused*/) {}

  template <auto Callable, typename payload_type>
  constexpr auto add(payload_type payload)
    requires(std::is_pointer_v<payload_type>)
  {
    std::tuple<payload_types..., payload_type> payloads =
        std::tuple_cat(m_payloads, std::make_tuple(payload));

    return DelegateChainBuilder<R(callable_args...),
                                TypeList<payload_types..., payload_type>,
                                callables..., Callable>{payloads};
  }

  template <auto Callable>
  constexpr auto add() {
    std::tuple<payload_types..., std::nullptr_t> payloads =
        std::tuple_cat(m_payloads, std::make_tuple(std::nullptr_t{}));

    return DelegateChainBuilder<R(callable_args...),
                                TypeList<payload_types..., std::nullptr_t>,
                                callables..., Callable>{payloads};
  }

  delegate_type build()
    requires(sizeof...(callables) > 0)
  {
    auto block = std::make_unique<const DispatchBlock>(m_payloads);
    delegate_type delegate;
    delegate.template connect<&DispatchBlock::dispatch>(std::move(block));
    return delegate;
  }

  void store(delegate_type& delegate)
    requires(sizeof...(callables) > 0)
  {
    auto block = std::make_unique<const DispatchBlock>(m_payloads);
    delegate.template connect<&DispatchBlock::dispatch>(std::move(block));
  }

  void store(Delegate<R(callable_args...)>& delegate)
    requires(sizeof...(callables) == 1)
  {
    constexpr auto callable =
        DispatchBlock::template findCallable<0, 0, callables...>();
    delegate.template connect<callable>(std::get<0>(m_payloads));
  }

 private:
  DelegateChainBuilder(std::tuple<payload_types...> payloads)
      : m_payloads(payloads) {}

  struct DispatchBlock {
    template <std::size_t I, std::size_t J, auto head, auto... tail>
    static constexpr auto findCallable() {
      if constexpr (I == J) {
        return head;
      } else {
        return findCallable<I, J + 1, tail...>();
      }
    }

    template <std::size_t I = 0, typename result_ptr>
    static constexpr auto invoke(result_ptr result, const tuple_type* payloads,
                                 callable_args... args) {
      const auto& callable = findCallable<I, 0, callables...>();

      if constexpr (!std::is_same_v<std::tuple_element_t<I, tuple_type>,
                                    std::nullptr_t>) {
        auto payload = std::get<I>(*payloads);

        if constexpr (!std::is_same_v<result_ptr, std::nullptr_t>) {
          std::get<I>(*result) = std::invoke(
              callable, payload, std::forward<callable_args>(args)...);
        } else {
          std::invoke(callable, payload, std::forward<callable_args>(args)...);
        }

      } else {
        if constexpr (!std::is_same_v<result_ptr, std::nullptr_t>) {
          std::get<I>(*result) =
              std::invoke(callable, std::forward<callable_args>(args)...);
        } else {
          std::invoke(callable, std::forward<callable_args>(args)...);
        }
      }

      if constexpr (I < sizeof...(payload_types) - 1) {
        invoke<I + 1>(result, payloads, std::forward<callable_args>(args)...);
      }
    }

    DispatchBlock(tuple_type payloads) : m_payloads(std::move(payloads)) {}

    tuple_type m_payloads{};

    auto dispatch(callable_args... args) const {
      if constexpr (std::is_same_v<R, void>) {
        invoke(nullptr, &m_payloads, std::forward<callable_args>(args)...);
      } else {
        static_assert(
            std::is_same_v<R, void> || std::is_default_constructible_v<R>,
            "Delegate chain return type must be void or default constructible");
        return_type result{};
        invoke(&result, &m_payloads, std::forward<callable_args>(args)...);
        return result;
      }
    }
  };

 private:
  tuple_type m_payloads{};
};

template <typename D>
DelegateChainBuilder(const D& /*unused*/)
    -> DelegateChainBuilder<typename D::signature_type>;

}  // namespace Acts
