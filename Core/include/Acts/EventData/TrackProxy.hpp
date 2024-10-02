// This file is part of the ACTS project.
//
// Copyright (C) 2016 CERN for the benefit of the ACTS project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at https://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Definitions/TrackParametrization.hpp"
#include "Acts/EventData/MultiTrajectory.hpp"
#include "Acts/EventData/MultiTrajectoryBackendConcept.hpp"
#include "Acts/EventData/ParticleHypothesis.hpp"
#include "Acts/EventData/TrackContainerBackendConcept.hpp"
#include "Acts/EventData/TrackParameters.hpp"
#include "Acts/EventData/TrackProxyConcept.hpp"
#include "Acts/EventData/TrackStatePropMask.hpp"
#include "Acts/Utilities/HashedString.hpp"
#include "Acts/Utilities/UnitVectors.hpp"

#include <iterator>
#include <type_traits>

namespace Acts {

template <TrackContainerBackend track_container_t,
          CommonMultiTrajectoryBackend traj_t,
          template <typename> class holder_t>
class TrackContainer;

namespace detail_tc {
template <typename T, bool select>
using ConstIf = std::conditional_t<select, const T, T>;

/// Helper iterator to allow iteration over tracks via track proxies.
template <typename container_t, typename proxy_t, bool ReadOnly>
class TrackProxyIterator {
  using ProxyType = proxy_t;
  using IndexType = typename ProxyType::IndexType;
  using ContainerType = container_t;

 public:
  using iterator_category = std::random_access_iterator_tag;
  using value_type = ProxyType;
  using difference_type = std::ptrdiff_t;
  using pointer = void;
  using reference = void;

  TrackProxyIterator(container_t& container, IndexType itrack)
    requires(!ReadOnly)
      : m_container(&container), m_itrack(itrack) {}

  TrackProxyIterator(const container_t& container, IndexType itrack)
    requires ReadOnly
      : m_container(&container), m_itrack(itrack) {}

  TrackProxyIterator& operator++() {
    m_itrack++;
    return *this;
  }
  TrackProxyIterator& operator--() {
    m_itrack--;
    return *this;
  }

  bool operator==(const TrackProxyIterator& other) const {
    return m_container == other.m_container && m_itrack == other.m_itrack;
  }

  auto operator<=>(const TrackProxyIterator& other) const {
    return m_itrack <=> other.m_itrack;
  }

  ProxyType operator*() const { return m_container->getTrack(m_itrack); }

  ProxyType operator*()
    requires(!ReadOnly)
  {
    return m_container->getTrack(m_itrack);
  }

  TrackProxyIterator operator[](difference_type n) const {
    TrackProxyIterator copy = *this;
    copy += n;
    return copy;
  };

  TrackProxyIterator& operator+=(difference_type n) {
    m_itrack += n;
    return *this;
  }

  TrackProxyIterator operator-=(difference_type n) {
    m_itrack -= n;
    return *this;
  }

  friend difference_type operator-(const TrackProxyIterator& lhs,
                                   const TrackProxyIterator& rhs) {
    return lhs.m_itrack - rhs.m_itrack;
  }

  friend TrackProxyIterator operator+(const TrackProxyIterator& lhs,
                                      difference_type rhs) {
    TrackProxyIterator copy = lhs;
    copy += rhs;
    return copy;
  }

  friend TrackProxyIterator operator+(difference_type lhs,
                                      const TrackProxyIterator& rhs) {
    return rhs + lhs;
  }

  friend TrackProxyIterator operator-(const TrackProxyIterator& lhs,
                                      difference_type rhs) {
    return lhs + (-rhs);
  }

  friend TrackProxyIterator operator-(difference_type lhs,
                                      const TrackProxyIterator& rhs) {
    return rhs + (-lhs);
  }

 private:
  detail_lt::TransitiveConstPointer<ConstIf<ContainerType, ReadOnly>>
      m_container;
  IndexType m_itrack;
};

}  // namespace detail_tc

/// Proxy class representing a single track.
/// This class provides a **view** into an associated @ref TrackContainer, and
/// has **reference semantics**. You can think of it as a pointer to a vector
/// of tracks, which exposes an object-oriented interface for accessing the
/// track properties.
///
/// @tparam track_container_t the container backend
/// @tparam trajectory_t the track state container backend
/// @tparam holder_t ownership management class for the backend
/// @tparam read_only true if this track container is not mutable
template <typename track_container_t, typename trajectory_t,
          template <typename> class holder_t, bool read_only = true>
class TrackProxy {
 public:
  /// Indicates whether this track proxy is read-only or if it can be modified
  static constexpr bool ReadOnly = read_only;

  /// The track container backend given as a template parameter
  using Container = track_container_t;

  /// The track state container backend given as a template parameter
  using Trajectory = trajectory_t;

  /// The index type of the track container
  using IndexType = typename Container::IndexType;

  /// Sentinel value that indicates an invalid index
  static constexpr IndexType kInvalid = Container::kInvalid;

  /// Alias for the mutable version of this track proxy, with the same backends
  using MutableTrackProxy =
      TrackProxy<track_container_t, trajectory_t, holder_t, false>;

  /// Alias for the const version of this track proxy, with the same backends
  using ConstTrackProxy =
      TrackProxy<track_container_t, trajectory_t, holder_t, true>;

  /// Alias for an associated const track proxy, with the same backends
  using ConstProxyType = ConstTrackProxy;

  /// Alias for an associated mutable track state proxy, with the same backends
  using TrackStateProxy = typename Trajectory::TrackStateProxy;

  /// Alias for an associated const track state proxy, with the same backends
  using ConstTrackStateProxy = typename Trajectory::ConstTrackStateProxy;

  /// Map-type for a bound parameter vector. This has reference semantics, i.e.
  /// points at a matrix by an internal pointer.
  using Parameters =
      typename detail_lt::FixedSizeTypes<eBoundSize, false>::CoefficientsMap;

  /// Same as @ref Parameters, but with const semantics
  using ConstParameters =
      typename detail_lt::FixedSizeTypes<eBoundSize, true>::CoefficientsMap;

  /// Map-type for a bound covariance. This has reference semantics, i.e.
  /// points at a matrix by an internal pointer.
  using Covariance =
      typename detail_lt::FixedSizeTypes<eBoundSize, false>::CovarianceMap;

  /// Same as @ref Covariance, but with const semantics
  using ConstCovariance =
      typename detail_lt::FixedSizeTypes<eBoundSize, true>::CovarianceMap;

#ifndef DOXYGEN
  friend TrackContainer<Container, Trajectory, holder_t>;
  friend MutableTrackProxy;
  friend ConstTrackProxy;
  // Track proxies are friends, not food!
  template <typename A, typename B, template <typename> class H, bool R>
  friend class TrackProxy;
#endif

  /// @anchor track_proxy_construct
  /// @name Constructors and assignment operator
  ///
  /// Public constructors and assignment operators for @c TrackProxy only
  /// allow construction from another @c TrackProxy. You should generally
  /// not have to construct @c TrackProxy manually.
  ///
  /// @{

  /// Copy constructor from a mutable track proxy. This is always valid, either
  /// mutable to mutable or mutable to const
  /// @param other the other track state proxy
  TrackProxy(const MutableTrackProxy& other)
      : m_container{other.m_container}, m_index{other.m_index} {}

  /// Copy assignment operator from mutable track proxy. This is always valid,
  /// either mutable to mutable or mutable to const
  /// @param other the other track state proxy
  TrackProxy& operator=(const MutableTrackProxy& other) {
    m_container = other.m_container;
    m_index = other.m_index;
    return *this;
  }

  /// @}

  /// Equality operator with another track proxy
  /// Checks the container identity and the track index
  /// @return True if the track proxies refer to the same track
  bool operator==(const TrackProxy& other) const {
    return &(*m_container) == &(*other.m_container) && m_index == other.m_index;
  }

  /// @anchor track_proxy_props
  /// @name TrackProxy properties
  /// Methods that give access to the properties of a track represented by
  /// @c TrackProxy.
  ///
  /// Many of these methods come in a @c const and a non-@c const version. The
  /// non-@c const version is only available if you have an instance of
  /// @c TrackProxy that does not have the @c read_only template parameter set to
  /// @c true, even if you hold it as an lvalue.
  ///
  /// @{

  /// Get the tip index, i.e. the entry point into the track state container
  /// @return the tip index by value
  IndexType tipIndex() const {
    return component<IndexType>(hashString("tipIndex"));
  }

  /// Index of the stem, i.e. the innermost track state of the track.
  /// This might be invalid, signifying that the track state is not
  /// forward-linked.
  /// @return the stem index
  IndexType stemIndex() const {
    return component<IndexType>(hashString("stemIndex"));
  }

  /// Get a mutable reference to the tip index, i.e. the entry point into the
  /// track container
  /// @note Only available if the track proxy is not read-only
  /// @return mutable reference to the tip index
  IndexType& tipIndex()
    requires(!ReadOnly)
  {
    return component<IndexType>(hashString("tipIndex"));
  }

  /// Index of the stem, i.e. the innermost track state of the track.
  /// This might be invalid, signifying that the track state is not
  /// forward-linked.
  /// @note Only available if the track proxy is not read-only
  /// @return mutable reference to the stem index
  IndexType& stemIndex()
    requires(!ReadOnly)
  {
    return component<IndexType>(hashString("stemIndex"));
  }

  /// Get the reference surface of the track (e.g. the perigee)
  /// @return the reference surface
  const Surface& referenceSurface() const {
    return *m_container->container().referenceSurface_impl(m_index);
  }

  // NOLINTBEGIN(performance-unnecessary-value-param)
  // looks like a false-positive. clang-tidy believes `srf` is not movable.
  /// Set a new reference surface for this track
  /// @param srf The surface to set
  void setReferenceSurface(std::shared_ptr<const Surface> srf)
    requires(!ReadOnly)
  {
    m_container->container().setReferenceSurface_impl(m_index, std::move(srf));
  }
  // NOLINTEND(performance-unnecessary-value-param)

  /// Returns whether the track has a reference surface or not
  /// @return whether a surface exists or not
  bool hasReferenceSurface() const {
    // @TODO: This could be more efficient
    return m_container->container().referenceSurface_impl(m_index) != nullptr;
  }

  /// Get the parameters of the track at the reference surface (e.g. perigee).
  /// Const version
  /// @return Proxy vector for the parameters
  ConstParameters parameters() const {
    return m_container->parameters(m_index);
  }

  /// Get the covariance of the track at the reference surface (e.g. perigee).
  /// Const version
  /// @return Proxy matrix for the covariance
  ConstCovariance covariance() const {
    return m_container->covariance(m_index);
  }

  /// Get the parameters of the track at the reference surface (e.g. perigee).
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return Proxy vector for the parameters
  Parameters parameters()
    requires(!ReadOnly)
  {
    return m_container->parameters(m_index);
  }

  /// Get the covariance of the track at the reference surface (e.g. perigee).
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return Proxy matrix for the covariance
  Covariance covariance()
    requires(!ReadOnly)
  {
    return m_container->covariance(m_index);
  }

  /// Access the theta parameter of the track at the reference surface
  /// @return The theta parameter
  ActsScalar theta() const { return parameters()[eBoundTheta]; }

  /// Access the phi parameter of the track at the reference surface
  /// @return The phi parameter
  ActsScalar phi() const { return parameters()[eBoundPhi]; }

  /// Access the loc0 parameter of the track at the reference surface
  /// @return The loc0 parameter
  ActsScalar loc0() const { return parameters()[eBoundLoc0]; }

  /// Access the loc1 parameter of the track at the reference surface
  /// @return The loc1 parameter
  ActsScalar loc1() const { return parameters()[eBoundLoc1]; }

  /// Access the time parameter of the track at the reference surface
  /// @return The time parameter
  ActsScalar time() const { return parameters()[eBoundTime]; }

  /// Access the q/p (curvature) parameter of the track at the reference surface
  /// @return The q/p parameter
  ActsScalar qOverP() const { return parameters()[eBoundQOverP]; }

  /// Get the particle hypothesis
  /// @return the particle hypothesis
  ParticleHypothesis particleHypothesis() const {
    return m_container->container().particleHypothesis_impl(m_index);
  }

  /// Set a new particle hypothesis for this track
  /// @note Only available if the track proxy is not read-only
  /// @param particleHypothesis The particle hypothesis to set
  void setParticleHypothesis(const ParticleHypothesis& particleHypothesis)
    requires(!ReadOnly)
  {
    m_container->container().setParticleHypothesis_impl(m_index,
                                                        particleHypothesis);
  }

  /// Get the charge of the tack
  /// @note this depends on the charge hypothesis
  /// @return The absolute track momentum
  ActsScalar charge() const { return particleHypothesis().qFromQOP(qOverP()); }

  /// Get the absolute momentum of the tack
  /// @return The absolute track momentum
  ActsScalar absoluteMomentum() const {
    return particleHypothesis().extractMomentum(qOverP());
  }

  /// Get the transverse momentum of the track
  /// @return The track transverse momentum value
  ActsScalar transverseMomentum() const {
    return std::sin(theta()) * absoluteMomentum();
  }

  /// Get a unit vector along the track direction at the reference surface
  /// @return The direction unit vector
  Vector3 direction() const {
    return makeDirectionFromPhiTheta(phi(), theta());
  }

  /// Get the global momentum vector
  /// @return the global momentum vector
  Vector3 momentum() const { return absoluteMomentum() * direction(); }

  /// Return the number of track states associated to this track
  /// @note This is calculated by iterating over the track states which is
  ///       somewhat expensive. Consider caching this value if you need It
  ///       more than once.
  /// @return The number of track states
  unsigned int nTrackStates() const {
    // @TODO: This should probably be cached, distance is expensive
    //        without random access
    if (tipIndex() == kInvalid) {
      // no tip index -> no track states
      return 0;
    }
    auto tsRange = trackStatesReversed();
    return std::distance(tsRange.begin(), tsRange.end());
  }

  /// Return the number of measurements for the track. Const version
  /// @note Only available if the track proxy is not read-only
  /// @return The number of measurements
  unsigned int& nMeasurements()
    requires(!ReadOnly)
  {
    return component<unsigned int, hashString("nMeasurements")>();
  }

  /// Return a mutable reference to the number of measurements for the track.
  /// Mutable version
  /// @return The number of measurements
  unsigned int nMeasurements() const {
    return component<unsigned int, hashString("nMeasurements")>();
  }

  /// Return a mutable reference to the number of holes for the track.
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return The number of holes
  unsigned int& nHoles()
    requires(!ReadOnly)
  {
    return component<unsigned int, hashString("nHoles")>();
  }

  /// Return the number of measurements for the track. Const version
  /// @return The number of measurements
  unsigned int nHoles() const {
    return component<unsigned int, hashString("nHoles")>();
  }

  /// Return a mutable reference to the number of outliers for the track.
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return The number of outliers
  unsigned int& nOutliers()
    requires(!ReadOnly)
  {
    return component<unsigned int, hashString("nOutliers")>();
  }

  /// Return the number of outliers for the track. Const version
  /// @return The number of outliers
  unsigned int nOutliers() const {
    return component<unsigned int, hashString("nOutliers")>();
  }

  /// Return a mutable reference to the number of shared hits for the track.
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return The number of shared hits
  unsigned int& nSharedHits()
    requires(!ReadOnly)
  {
    return component<unsigned int, hashString("nSharedHits")>();
  }

  /// Return the number of shared hits for the track. Const version
  /// @return The number of shared hits
  unsigned int nSharedHits() const {
    return component<unsigned int, hashString("nSharedHits")>();
  }

  /// Return a mutable reference to the chi squared
  /// Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return The chi squared
  float& chi2()
    requires(!ReadOnly)
  {
    return component<float, hashString("chi2")>();
  }

  /// Return the chi squared for the track. Const version
  /// @return The chi squared
  float chi2() const { return component<float, hashString("chi2")>(); }

  /// Return a mutable reference to the number of degrees of freedom for the
  /// track. Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @return The number of degrees of freedom
  unsigned int& nDoF()
    requires(!ReadOnly)
  {
    return component<unsigned int, hashString("ndf")>();
  }

  /// Return the number of degrees of freedom for the track. Const version
  /// @return The number of degrees of freedom
  unsigned int nDoF() const {
    return component<unsigned int, hashString("ndf")>();
  }

  /// Return the index of this track in the track container
  /// @note This is separate from the tip index
  /// @return the track index
  IndexType index() const { return m_index; }

  /// @}

  /// @anchor track_proxy_track_states
  /// @name TrackProxy track state access
  /// Methods that give access to the track states of a track represented by @c TrackProxy.
  /// @{

  /// Return a const track state proxy to the outermost track state
  /// @return The outermost track state proxy
  ConstTrackStateProxy outermostTrackState() const {
    return m_container->trackStateContainer().getTrackState(tipIndex());
  }

  /// Return a mutable track state proxy to the outermost track state
  /// @return The outermost track state proxy
  TrackStateProxy outermostTrackState()
    requires(!ReadOnly)
  {
    return m_container->trackStateContainer().getTrackState(tipIndex());
  }

  /// Return a const track state proxy to the innermost track state
  /// @note This is only available, if the track is forward linked
  /// @return The innermost track state proxy
  auto innermostTrackState() const {
    using proxy_t = decltype(m_container->trackStateContainer().getTrackState(
        std::declval<IndexType>()));

    IndexType stem = component<IndexType, hashString("stemIndex")>();
    if (stem == kInvalid) {
      return std::optional<proxy_t>{};
    } else {
      return std::optional<proxy_t>{
          m_container->trackStateContainer().getTrackState(stem)};
    }
  }

  /// Return a mutable track state proxy to the innermost track state
  /// @note This is only available, if the track is forward linked
  /// @note Only available if the track proxy is not read-only
  /// @return The innermost track state proxy
  auto innermostTrackState()
    requires(!ReadOnly)
  {
    using proxy_t = decltype(m_container->trackStateContainer().getTrackState(
        std::declval<IndexType>()));

    IndexType stem = component<IndexType>(hashString("stemIndex"));
    if (stem == kInvalid) {
      return std::optional<proxy_t>{};
    } else {
      return std::optional<proxy_t>{
          m_container->trackStateContainer().getTrackState(stem)};
    }
  }

  /// Get a range over the track states of this track. Return value is
  /// compatible with range based for loop. Const version
  /// @note This range is from the outside inwards!
  /// @return Track state range to iterate over
  auto trackStatesReversed() const {
    return m_container->reverseTrackStateRange(m_index);
  }

  /// Get a range over the track states of this track. Return value is
  /// compatible with range based for loop. Mutable version
  /// @note Only available if the track proxy is not read-only
  /// @note This range is from the outside inwards!
  /// @return Track state range to iterate over
  auto trackStatesReversed()
    requires(!ReadOnly)
  {
    return m_container->reverseTrackStateRange(m_index);
  }

  /// Get a range over the track states of this track. Return value is
  /// compatible with range based for loop. This overload returns a const-only
  /// track state range, which means you cannot modify the track states obtained
  /// in the iteration.
  /// @note This range is from the inside out!
  /// @warning This access direction is only possible if the track states are
  ///          **forward-linked**.
  /// @return Track state range to iterate over
  auto trackStates() const {
    return m_container->forwardTrackStateRange(m_index);
  }

  /// Get a range over the track states of this track.
  /// Return value is compatible with range based for loop.
  /// This overload returns a mutable track state range, which means you
  /// can modify the track states obtained in the iteration.
  /// @note Only available if the track proxy is not read-only
  /// @note This range is from the inside out!
  /// @warning This access direction is only possible if the track states are
  ///          **forward-linked**.
  /// @return Track state range to iterate over
  auto trackStates()
    requires(!ReadOnly)
  {
    return m_container->forwardTrackStateRange(m_index);
  }

  /// @}

  /// @anchor track_proxy_track_state_manipulation
  /// @name TrackProxy track state manipulation
  /// Methods that manipulate the track states of a track represented by @c TrackProxy.
  /// @{

  /// Forward connect a track.
  /// This means setting indices from the inside out on all track states.
  /// @note Only available if the track proxy is not read-only
  void linkForward()
    requires(!ReadOnly)
  {
    IndexType last = kInvalid;
    for (auto ts : trackStatesReversed()) {
      ts.template component<IndexType>(hashString("next")) = last;
      last = ts.index();
    }
    stemIndex() = last;
  }

  /// Append a track state to this track.
  /// This will modify the tip index to point at the newly created track state,
  /// which will be directly after the previous track state at tip index.
  /// @note Only available if the track proxy is not read-only
  /// @param mask The allocation prop mask for the new track state
  /// @return The newly added track state
  auto appendTrackState(TrackStatePropMask mask = TrackStatePropMask::All)
    requires(!ReadOnly)
  {
    auto& tsc = m_container->trackStateContainer();
    auto ts = tsc.makeTrackState(mask, tipIndex());
    tipIndex() = ts.index();
    return ts;
  }

  /// Copy the content of another track proxy into this one
  /// @note Only available if the track proxy is not read-only
  /// @tparam track_proxy_t the other track proxy's type
  /// @param other The track proxy
  /// @param copyTrackStates Copy the track state sequence from @p other
  template <TrackProxyConcept track_proxy_t>
  void copyFrom(const track_proxy_t& other, bool copyTrackStates = true)
    requires(!ReadOnly)
  {
    // @TODO: Add constraint on which track proxies are allowed,
    // this is only implicit right now

    if (copyTrackStates) {
      // append track states (cheap), but they're in the wrong order
      for (const auto& srcTrackState : other.trackStatesReversed()) {
        auto destTrackState = appendTrackState(srcTrackState.getMask());
        if (srcTrackState.hasCalibrated()) {
          destTrackState.allocateCalibrated(srcTrackState.calibratedSize());
        }
        destTrackState.copyFrom(srcTrackState, Acts::TrackStatePropMask::All,
                                true);
      }

      // reverse using standard linked list reversal algorithm
      reverseTrackStates();
    }

    setParticleHypothesis(other.particleHypothesis());

    if (other.hasReferenceSurface()) {
      setReferenceSurface(other.referenceSurface().getSharedPtr());
      parameters() = other.parameters();
      covariance() = other.covariance();
    } else {
      setReferenceSurface(nullptr);
    }

    nMeasurements() = other.nMeasurements();
    nHoles() = other.nHoles();
    nOutliers() = other.nOutliers();
    nSharedHits() = other.nSharedHits();
    chi2() = other.chi2();
    nDoF() = other.nDoF();

    m_container->copyDynamicFrom(m_index, other.m_container->container(),
                                 other.m_index);
  }

  /// Creates  a *shallow copy* of the track. Track states are not copied, but
  /// the resulting track points at the same track states as the original.
  /// @note Only available if the track proxy is not read-only
  TrackProxy shallowCopy()
    requires(!ReadOnly)
  {
    auto ts = container().makeTrack();
    ts.copyFrom(*this, false);
    ts.tipIndex() = tipIndex();
    ts.stemIndex() = stemIndex();
    return ts;
  }

  /// Reverse the ordering of track states for this track
  /// Afterwards, the previous endpoint of the track state sequence will be the
  /// "innermost" track state
  /// @note Only available if the track proxy is not read-only
  /// @note This is dangerous with branching track state sequences, as it will break them
  /// @note This also automatically forward-links the track!
  /// @param invertJacobians Whether to invert the Jacobians of the track states
  void reverseTrackStates(bool invertJacobians = false)
    requires(!ReadOnly)
  {
    IndexType current = tipIndex();
    IndexType next = kInvalid;
    IndexType prev = kInvalid;

    stemIndex() = tipIndex();

    // @TODO: Maybe refactor to not need this variable if invertJacobians == false
    BoundMatrix nextJacobian;

    while (current != kInvalid) {
      auto ts = m_container->trackStateContainer().getTrackState(current);
      prev = ts.previous();
      ts.template component<IndexType>(hashString("next")) = prev;
      ts.previous() = next;
      if (invertJacobians) {
        if (next != kInvalid) {
          BoundMatrix curJacobian = ts.jacobian();
          ts.jacobian() = nextJacobian.inverse();
          nextJacobian = curJacobian;
        } else {
          nextJacobian = ts.jacobian();
          ts.jacobian().setZero();
        }
      }
      next = current;
      tipIndex() = current;
      current = prev;
    }
  }

  /// @}

  /// @anchor track_proxy_generic_component
  /// @name TrackProxy generic component access
  /// Methods that give access to generic components of a track represented by
  /// @c TrackProxy.  Internally, a compile-time hash of the component name is
  /// used to identify which component is being requested. Most of the named
  /// methods in @ref track_proxy_props "TrackProxy properties" use these
  /// methods to retrieve the actual data.
  ///
  /// A number of overloads exist, where you can either supply the
  /// @ref HashedString @c key as a template parameter or a runtime argument.  The
  /// former has the advantage of being guaranteed to be evaluated at
  /// compile-time.
  ///
  /// @{

  /// Retrieve a mutable reference to a component
  /// @tparam T The type of the component to access
  /// @tparam key String key for the component to access
  /// @return Mutable reference to the component given by @p key
  template <typename T, HashedString key>
  constexpr T& component()
    requires(!ReadOnly)
  {
    return m_container->template component<T, key>(m_index);
  }

  /// Retrieve a mutable reference to a component
  /// @tparam T The type of the component to access
  /// @param key String key for the component to access
  /// @return Mutable reference to the component given by @p key
  template <typename T>
  constexpr T& component(HashedString key)
    requires(!ReadOnly)
  {
    return m_container->template component<T>(key, m_index);
  }

  /// Retrieve a mutable reference to a component
  /// @tparam T The type of the component to access
  /// @param key String key for the component to access
  /// @note This might hash the @p key at runtime instead of compile-time
  /// @return Mutable reference to the component given by @p key
  template <typename T>
  constexpr T& component(std::string_view key)
    requires(!ReadOnly)
  {
    return m_container->template component<T>(hashString(key), m_index);
  }

  /// Retrieve a const reference to a component
  /// @tparam T The type of the component to access
  /// @tparam key String key for the component to access
  /// @return Const reference to the component given by @p key
  template <typename T, HashedString key>
  constexpr const T& component() const {
    return m_container->template component<T, key>(m_index);
  }

  /// Retrieve a const reference to a component
  /// @tparam T The type of the component to access
  /// @param key String key for the component to access
  /// @return Const reference to the component given by @p key
  template <typename T>
  constexpr const T& component(HashedString key) const {
    return m_container->template component<T>(key, m_index);
  }

  /// Retrieve a const reference to a component
  /// @tparam T The type of the component to access
  /// @param key String key for the component to access
  /// @note This might hash the @p key at runtime instead of compile-time
  /// @return Const reference to the component given by @p key
  template <typename T>
  constexpr const T& component(std::string_view key) const {
    return m_container->template component<T>(hashString(key), m_index);
  }

  /// @}

  /// Return the track parameters at the reference surface
  /// @note The parameters are created on the fly
  /// @return the track parameters
  BoundTrackParameters createParametersAtReference() const {
    return BoundTrackParameters(referenceSurface().getSharedPtr(), parameters(),
                                covariance(), particleHypothesis());
  }

  /// Convert a track state into track parameters
  /// @note The parameters are created on the fly
  /// @return the track parameters
  BoundTrackParameters createParametersFromState(
      const ConstTrackStateProxy& trackState) const {
    return BoundTrackParameters(trackState.referenceSurface().getSharedPtr(),
                                trackState.parameters(),
                                trackState.covariance(), particleHypothesis());
  }

  /// Return a reference to the track container backend, mutable version.
  /// @note Only available if the track proxy is not read-only
  /// @return reference to the track container backend
  auto& container()
    requires(!ReadOnly)
  {
    return *m_container;
  }

  /// Return a reference to the track container backend, const version.
  /// @return reference to the track container backend
  const auto& container() const { return *m_container; }

 private:
  TrackProxy(detail_tc::ConstIf<TrackContainer<Container, Trajectory, holder_t>,
                                ReadOnly>& container,
             IndexType itrack)
      : m_container{&container}, m_index{itrack} {}

  detail_lt::TransitiveConstPointer<detail_tc::ConstIf<
      TrackContainer<Container, Trajectory, holder_t>, ReadOnly>>
      m_container;
  IndexType m_index;
};

}  // namespace Acts
