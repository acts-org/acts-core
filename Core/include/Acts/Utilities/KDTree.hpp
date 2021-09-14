// This file is part of the Acts project.
//
// Copyright (C) 2021 CERN for the benefit of the Acts project
//
// This Source Code Form is subject to the terms of the Mozilla Public
// License, v. 2.0. If a copy of the MPL was not distributed with this
// file, You can obtain one at http://mozilla.org/MPL/2.0/.

#pragma once

#include "Acts/Utilities/RangeXD.hpp"

#include <algorithm>
#include <array>
#include <cmath>

namespace Acts {
/// @brief A general k-d tree with fast range search.
///
/// This is a generalized k-d tree, with a configurable number of dimension,
/// scalar type, content type, index type, vector type, and leaf size. This
/// class is purposefully generalized to support a wide range of use cases.
///
/// A k-d tree is, in essence, a k-dimensional binary search tree. Each interal
/// node splits the content of the tree in half, with the pivot point being an
/// orthogonal hyperplane in one of the k dimensions. This allows us to
/// efficiently look up points within certain k-dimensional ranges.
///
/// This particular class is mostly a wrapper class around an underlying node
/// class which does all the actual work.
///
/// @note This type is completely immutable after construction.
///
/// @tparam Dims The number of dimensions.
/// @tparam Type The type of value contained in the tree.
/// @tparam Scalar The scalar type used to construct position vectors.
/// @tparam Vector The general vector type used to construct coordinates.
/// @tparam LeafSize The maximum number of elements stored in a leaf node.
template <std::size_t Dims, typename Type, typename Scalar = double,
          template <typename, std::size_t> typename Vector = std::array,
          std::size_t LeafSize = 4>
class KDTree {
 public:
  /// @brief The type of value contained in this k-d tree.
  using value_t = Type;

  /// @brief The type describing a multi-dimensional orthogonal range.
  using range_t = RangeXD<Dims, Scalar, Vector>;

  /// @brief The type of coordinates for points.
  using coordinate_t = Vector<Scalar, Dims>;

  /// @brief The type of coordinate-value pairs.
  using pair_t = std::pair<coordinate_t, Type>;

  /// @brief The type of a vector of coordinate-value pairs.
  using vector_t = std::vector<pair_t>;

  /// @brief The type of iterators in our vectors.
  using iterator_t = typename vector_t::iterator;

  // We do not need an empty constructor - this is never useful.
  KDTree() = delete;

  /// @brief Construct a k-d tree from a vector of position-value pairs.
  ///
  /// This constructor takes an r-value reference to a vector of position-value
  /// pairs and constructs a k-d tree from those pairs.
  ///
  /// @param d The vector of position-value pairs to construct the k-d tree
  /// from.
  KDTree(vector_t &&d) : m_elems(d) {
    // To start out, we need to check whether we need to construct a leaf node
    // or an internal node. We create a leaf only if we have at most as many
    // elements as the number of elements that can fit into a leaf node.
    // Hopefully most invocations of this constructor will have more than a few
    // elements!
    //
    // One interesting thing to note is that all of the nodes in the k-d tree
    // have a range in the element vector of the outermost node. They simply
    // make in-place changes to this array, and they hold no memory of their
    // own.
    if (m_elems.size() > LeafSize) {
      m_root = std::make_unique<KDTreeNode>(m_elems.begin(), m_elems.end());
    } else {
      m_root = std::make_unique<KDTreeLeaf>(m_elems.begin(), m_elems.end());
    }
  }

  /// @brief Perform an orthogonal range search within the k-d tree.
  ///
  /// A range search operation is one that takes a k-d tree and an orthogonal
  /// range, and returns all values associated with coordinates in the k-d tree
  /// that lie within the orthogonal range. k-d trees can do this operation
  /// quickly.
  ///
  /// @param r The range to search for.
  ///
  /// @return The vector of all values that lie within the given range.
  std::vector<Type> rangeSearch(const range_t &r) const {
    std::vector<Type> out;

    rangeSearch(r, out);

    return out;
  }

  /// @brief Perform an orthogonal range search within the k-d tree, returning
  /// keys as well as values.
  ///
  /// Performs the same logic as the keyless version, but includes a copy of
  /// the key with each element.
  ///
  /// @param r The range to search for.
  ///
  /// @return The vector of all key-value pairs that lie within the given
  /// range.
  std::vector<pair_t> rangeSearchWithKey(const range_t &r) const {
    std::vector<pair_t> out;

    rangeSearchWithKey(r, out);

    return out;
  }

  /// @brief Perform an in-place orthogonal range search within the k-d tree.
  ///
  /// This range search module operates in place, writing its results to the
  /// given output vector.
  ///
  /// @param r The range to search for.
  /// @param v The vector to write the output to.
  void rangeSearch(const range_t &r, std::vector<Type> &v) const {
    rangeSearchInserter(r, std::back_inserter(v));
  }

  /// @brief Perform an in-place orthogonal range search within the k-d tree,
  /// including keys in the result.
  ///
  /// Performs the same operation as the keyless version, but includes the keys
  /// in the results.
  ///
  /// @param r The range to search for.
  /// @param v The vector to write the output to.
  void rangeSearchWithKey(const range_t &r, std::vector<pair_t> &v) const {
    rangeSearchInserterWithKey(r, std::back_inserter(v));
  }

  /// @brief Perform an orthogonal range search within the k-d tree, writing
  /// the resulting values to an output iterator.
  ///
  /// This method allows the user more control in where the result is written
  /// to.
  ///
  /// @tparam OutputIt The type of the output iterator.
  ///
  /// @param r The range to search for.
  /// @param i The iterator to write the output to.
  template <typename OutputIt>
  void rangeSearchInserter(const range_t &r, OutputIt i) const {
    rangeSearchMapDiscard(
        r, [i](const coordinate_t &, const Type &v) mutable { i = v; });
  }

  /// @brief Perform an orthogonal range search within the k-d tree, writing
  /// the resulting values to an output iterator, including the keys.
  ///
  /// Performs the same operation as the keyless version, but includes the key
  /// in the output.
  ///
  /// @tparam OutputIt The type of the output iterator.
  ///
  /// @param r The range to search for.
  /// @param i The iterator to write the output to.
  template <typename OutputIt>
  void rangeSearchInserterWithKey(const range_t &r, OutputIt i) const {
    rangeSearchMapDiscard(r, [i](const coordinate_t &c, const Type &v) mutable {
      i = {c, v};
    });
  }

  /// @brief Perform an orthogonal range search within the k-d tree, applying
  /// a mapping function to the values found.
  ///
  /// In some cases, we may wish to transform the values in some way. This
  /// method allows the user to provide a mapping function which takes a set of
  /// coordinates and a value and transforms them to a new value, which is
  /// returned.
  ///
  /// @note Your compiler may not be able to deduce the result type
  /// automatically, in which case you will need to specify it manually.
  ///
  /// @tparam Result The return type of the map operation.
  ///
  /// @param r The range to search for.
  /// @param f The mapping function to apply to key-value pairs.
  ///
  /// @return A vector of elements matching the range after the application of
  /// the mapping function.
  template <typename Result>
  std::vector<Result> rangeSearchMap(
      const range_t &r,
      std::function<Result(const coordinate_t &, const Type &)> f) const {
    std::vector<Result> out;

    rangeSearchMapInserter(r, f, std::back_inserter(out));

    return out;
  }

  /// @brief Perform an orthogonal range search within the k-d tree, applying a
  /// mapping function to the values found, and inserting them into an
  /// inserter.
  ///
  /// Performs the same operation as the interter-less version, but allows the
  /// user additional control over the insertion process.
  ///
  /// @note Your compiler may not be able to deduce the result type
  /// automatically, in which case you will need to specify it manually.
  ///
  /// @tparam Result The return type of the map operation.
  /// @tparam OutputIt The type of the output iterator.
  ///
  /// @param r The range to search for.
  /// @param f The mapping function to apply to key-value pairs.
  /// @param i The inserter to insert the results into.
  template <typename Result, typename OutputIt>
  void rangeSearchMapInserter(
      const range_t &r,
      std::function<Result(const coordinate_t &, const Type &)> f,
      OutputIt i) const {
    rangeSearchMapDiscard(r, [i, f](const coordinate_t &c,
                                    const Type &v) mutable { i = f(c, v); });
  }

  /// @brief Perform an orthogonal range search within the k-d tree, applying a
  /// a void-returning function with side-effects to each key-value pair.
  ///
  /// This is the most general version of range search in this class, and every
  /// other operation can be reduced to this operation as long as we allow
  /// arbitrary side-effects.
  ///
  /// Functional programmers will know this method as mapM_.
  ///
  /// @param r The range to search for.
  /// @param f The mapping function to apply to key-value pairs.
  void rangeSearchMapDiscard(
      const range_t &r,
      std::function<void(const coordinate_t &, const Type &)> f) const {
    m_root->rangeSearchMapDiscard(r, f);
  }

  /// @brief Return the number of elements in the k-d tree.
  ///
  /// We simply defer this method to the root node of the k-d tree.
  ///
  /// @return The number of elements in the k-d tree.
  std::size_t size(void) const { return m_root->size(); }

  /// @brief Return a string representing the structure of the k-d tree.
  ///
  /// Used mostly for debugging purposes. You probably do not want to call this
  /// in actual code.
  std::string toString(void) const { return m_root->toString(0); }

 private:
  static Scalar nextRepresentable(Scalar v) {
    // I'm not super happy with this bit of code, but since 1D ranges are
    // semi-open, we can't simply incorporate values by setting the maximum to
    // them. Instead, what we need to do is get the next representable value.
    // For integer values, this means adding one. For floating point types, we
    // rely on the nextafter method to get the smallest possible value that is
    // larger than the one we requested.
    if constexpr (std::is_integral_v<Scalar>) {
      return v + 1;
    } else if constexpr (std::is_floating_point_v<Scalar>) {
      return std::nextafter(v, std::numeric_limits<Scalar>::max());
    }
  }

  static range_t boundingBox(iterator_t b, iterator_t e) {
    // Firstly, we find the minimum and maximum value in each dimension to
    // construct a bounding box around this node's values.
    std::array<Scalar, Dims> min_v, max_v;

    for (std::size_t i = 0; i < Dims; ++i) {
      min_v[i] = std::numeric_limits<Scalar>::max();
      max_v[i] = std::numeric_limits<Scalar>::lowest();
    }

    for (iterator_t i = b; i != e; ++i) {
      for (std::size_t j = 0; j < Dims; ++j) {
        min_v[j] = std::min(min_v[j], i->first[j]);
        max_v[j] = std::max(max_v[j], i->first[j]);
      }
    }

    // Then, we construct a k-dimensional range from the given minima and
    // maxima, which again is just a bounding box.
    range_t r;

    for (std::size_t j = 0; j < Dims; ++j) {
      r[j] = {min_v[j], nextRepresentable(max_v[j])};
    }

    return r;
  }

  /// @brief An abstract class containing common features of k-d tree node
  /// types.
  ///
  /// A k-d tree consists of two different node types: leaf nodes and inner
  /// nodes. These nodes have some common functionality, which is captured by
  /// this common parent node type.
  class KDTreeAbstractNode {
   public:
    /// @brief Construct the common data for all node types.
    ///
    /// The node types share a few concepts, like an n-dimensional range, and a
    /// begin and end of the range of elements managed. This constructor
    /// calculates these things so that the individual child constructors don't
    /// have to.
    KDTreeAbstractNode(iterator_t _b, iterator_t _e)
        : m_begin_it(_b),
          m_end_it(_e),
          m_range(boundingBox(m_begin_it, m_end_it)) {}

    /// @brief Destroy this node in the k-d tree, and indirectly all of its
    /// children.
    virtual ~KDTreeAbstractNode(void) = default;

    /// @brief Perform a range search in the k-d tree, mapping the key-value
    /// pairs to a side-effecting function.
    ///
    /// This is the most powerful range search method we have, assuming that we
    /// can use arbitrary side effects, which we can. All other range search
    /// methods are implemented in terms of this particular function.
    ///
    /// @param r The range to search for.
    /// @param f The mapping function to apply to matching elements.
    virtual void rangeSearchMapDiscard(
        const range_t &r,
        std::function<void(const coordinate_t &, const Type &)> f) const = 0;

    /// @brief Debugging method that gives a textual representation of a k-d
    /// tree.
    ///
    /// It is probably best not to use this method in real code. It is designed
    /// for debugging purposes only.
    ///
    /// @param d The depth of this node (for indentation).
    virtual std::string toString(std::size_t d) const = 0;

    /// @brief Determine the number of elements managed by this node.
    ///
    /// Conveniently, this number is always equal to the distance between the
    /// begin iterator and the end iterator, so we can simply delegate to the
    /// relevant standard library method.
    ///
    /// @return The number of elements below this node.
    std::size_t size(void) const { return std::distance(m_begin_it, m_end_it); }

    /// @brief The axis-aligned bounding box containing all elements in this
    /// node.
    ///
    /// @return The minimal axis-aligned bounding box that contains all the
    /// elements under this node.
    const range_t &range(void) const { return m_range; }

   protected:
    /// @brief The start and end of the range of coordinate-value pairs under
    /// this node.
    const iterator_t m_begin_it, m_end_it;

    /// @brief The axis-aligned bounding box of the coordinates under this
    /// node.
    const range_t m_range;
  };

  /// @brief A non-leaf node in the k-d tree.
  ///
  /// These nodes are not the direct parents of any coordinate-value pairs, but
  /// rather they are the parents of two other nodes. This means that this type
  /// of node represents a split of the candidate space across a particular
  /// dimension.
  class KDTreeNode final : public KDTreeAbstractNode {
   public:
    /// @brief Constract an internal node from a range of coordinate-value
    /// pairs.
    ///
    /// The element range passed to this constructor is a subrange of the set
    /// of coordinate-value pairs owned by the outer-most parent node. The
    /// constructor rearranges the elements of this range.
    ///
    /// @param _b The iterator for the start of the range of elements.
    /// @param _e The iterator for the end of the range of elements.
    KDTreeNode(iterator_t _b, iterator_t _e) : KDTreeAbstractNode(_b, _e) {
      // This constant determines the maximum number of elements where we still
      // calculate the exact median of the values for the purposes of
      // splitting. In general, the closer the pivot value is to the true
      // median, the more balanced the tree will be. However, calculating the
      // median exactly is an O(n log n) operation, while approximating it is
      // an O(1) time.
      constexpr std::size_t max_exact_median = 128;

      // The following set of operations is designed to find the variance (of
      // normalized values) of the values in each of the dimensions. We do this
      // because the optimal way of constructing a k-d tree is to select the
      // pivot dimension as the dimension with the highest variance. To this
      // end, we first start by calculating the sum of all the values.
      std::array<Scalar, Dims> sum_v;

      sum_v.fill(0.0);

      for (iterator_t i = this->m_begin_it; i != this->m_end_it; ++i) {
        for (std::size_t j = 0; j < Dims; ++j) {
          // We normalize the values to be in the [0, 1] range, because
          // otherwise the dimension with the higher valued elements would
          // always win.
          Scalar value = (i->first[j] - this->m_range[j].min()) /
                         (this->m_range[j].max() - this->m_range[j].min());
          sum_v[j] += value;
        }
      }

      // Using the sum in each dimension, we can now calculate the mean in each
      // dimension.
      std::array<Scalar, Dims> mean_v;

      for (std::size_t j = 0; j < Dims; ++j) {
        mean_v[j] = sum_v[j] / this->size();
      }

      // Next, we calculate the summed squared error from the mean in each
      // dimension, again with the normalized values.
      std::array<Scalar, Dims> sqe_v;

      for (iterator_t i = this->m_begin_it; i != this->m_end_it; ++i) {
        for (std::size_t j = 0; j < Dims; ++j) {
          Scalar value = (i->first[j] - this->m_range[j].min()) /
                         (this->m_range[j].max() - this->m_range[j].min());
          sqe_v[j] += std::pow(value - mean_v[j], 2.0);
        }
      }

      // Finally, we calculate the variance of the elements in each dimension.
      std::array<Scalar, Dims> var_v;

      for (std::size_t j = 0; j < Dims; ++j) {
        var_v[j] = sqe_v[j] / this->size();
      }

      // Next, we find the dimension with the highest variance, and we will
      // keep that as our pivot dimension.
      m_dim = 0;

      for (std::size_t j = 0; j < Dims; ++j) {
        if (var_v[j] > var_v[m_dim]) {
          m_dim = j;
        }
      }

      // Next, we need to determine the pivot point of this node, that is to
      // say the point in the selected pivot dimension along which point we
      // will split the range. To do this, we check how large the set of
      // elements is. If it is sufficiently small, we use the median. Otherwise
      // we use the mean.
      if (this->size() > max_exact_median) {
        // In this case, we have a lot of elemenents, and sorting the range to
        // find the true median might be too expensive. Therefore, we will just
        // use the middle value between the minimum and maximum. This is not
        // nearly as accurate as using the median, but it's a nice cheat.
        m_mid = 0.5 * (this->m_range[m_dim].max() + this->m_range[m_dim].min());
      } else {
        // If the number of elements is fairly small, we will just calculate
        // the median exactly. We do this by finding the values in the
        // dimension, sorting it, and then taking the middle one.
        std::vector<Scalar> idxs;
        idxs.reserve(this->size());

        for (iterator_t i = this->m_begin_it; i != this->m_end_it; ++i) {
          idxs.push_back(i->first[m_dim]);
        }

        std::sort(idxs.begin(), idxs.end());
        m_mid = idxs[idxs.size() / 2];
      }

      // This is the meat of the pudding of our tree creation algorithm. We
      // partition the range using `std::partition`, which means we put all the
      // values lower than the midpoint on the left side of the range, and all
      // the values that are higher on the right. Then we get the pivot
      // iterator, which gives us the middle point.
      auto pivot = std::partition(
          this->m_begin_it, this->m_end_it,
          [=](const pair_t &i) { return i.first[m_dim] < m_mid; });

      // This should never really happen, but in very select cases where there
      // are a lot of equal values in the range, the pivot can end up all the
      // way at the end of the array and we end up in an infinite loop. We
      // check for pivot points which would not split the range, and fix them
      // if they occur.
      if (pivot == this->m_begin_it || pivot == std::prev(this->m_end_it)) {
        pivot = std::next(this->m_begin_it, LeafSize);
      }

      // Calculate the number of elements on the left-hand side, as well as the
      // right-hand side. We do this by calculating the difference from the
      // begin and end of the array to the pivot point.
      std::size_t lhs_size = std::distance(this->m_begin_it, pivot);
      std::size_t rhs_size = std::distance(pivot, this->m_end_it);

      // Next, we check whether the left-hand node should be another internal
      // node or a leaf node, and we construct the node recursively.
      if (lhs_size > LeafSize) {
        m_lhs = std::make_unique<KDTreeNode>(this->m_begin_it, pivot);
      } else if (lhs_size > 0) {
        m_lhs = std::make_unique<KDTreeLeaf>(this->m_begin_it, pivot);
      }

      // Same on the right hand side.
      if (rhs_size > LeafSize) {
        m_rhs = std::make_unique<KDTreeNode>(pivot, this->m_end_it);
      } else if (rhs_size > 0) {
        m_rhs = std::make_unique<KDTreeLeaf>(pivot, this->m_end_it);
      }
    }

    /// @brief Perform a range search in this (sub-)k-d tree.
    ///
    /// Performing a range search on an inner node is essentially the same as
    /// performing that same range search on both its children and appending
    /// them. However, we can also do some optimisations.
    ///
    /// @param r The orthogonal range to search for.
    /// @param f The mapping function to execute.
    virtual void rangeSearchMapDiscard(
        const range_t &r,
        std::function<void(const coordinate_t &, const Type &)> f)
        const override {
      // Firstly, we can check if the range completely contains the bounding
      // box of this node. If that is the case, we know for certain that any
      // value contained below this node should end up in the output, and we
      // can stop recursively looking for them.
      if (r >= this->m_range) {
        // We can also pre-allocate space for the number of elements, since we
        // are inserting all of them anyway.
        for (iterator_t i = this->m_begin_it; i != this->m_end_it; ++i) {
          f(i->first, i->second);
        }

        return;
      }

      // If we have a left-hand node (which we should!), then we check if there
      // is any overlap between the target range and the bounding box of the
      // left-hand node. If there is, we recursively search in that node.
      if (m_lhs && (m_lhs->range() && r)) {
        m_lhs->rangeSearchMapDiscard(r, f);
      }

      // Then, we perform exactly the same procedure for the right hand side.
      if (m_rhs && (m_rhs->range() && r)) {
        m_rhs->rangeSearchMapDiscard(r, f);
      }
    }

    /// @brief Debugging string method for this (sub-)k-d tree.
    ///
    /// This prints information to stdout about the structure of the (sub-)tree
    /// defined by this node. Not designed for use in real code.
    ///
    /// @param i The amount of indentation to use.
    virtual std::string toString(std::size_t i) const override {
      std::stringstream out;

      // First, we print some indentation to make the output look nicer.
      for (std::size_t j = 0; j < i; ++j) {
        out << " ";
      }

      // Print some information about the node.
      out << "Node with range " << this->m_range[m_dim].min() << " to "
          << this->m_mid << " to " << this->m_range[m_dim].max()
          << " (total size " << this->size() << ")" << std::endl;

      // Then, recursively print the left-hand side...
      if (m_lhs) {
        out << m_lhs->toString(i + 1);
      }

      // ...and the right-hand side.
      if (m_rhs) {
        out << m_rhs->toString(i + 1);
      }

      return out.str();
    }

   private:
    /// @brief Pointers to the left and right children.
    std::unique_ptr<KDTreeAbstractNode> m_lhs, m_rhs;

    /// @brief The index of the pivot dimension.
    std::size_t m_dim;

    /// @brief The value of the pivot in the pivot dimension.
    Scalar m_mid;
  };

  class KDTreeLeaf final : public KDTreeAbstractNode {
   public:
    /// @brief Construct a leaf node from a range of elements.
    ///
    /// This doesn't actually do anything that the abstract base constructor
    /// doesn't do.
    ///
    /// @param _b The begin iterator of the range of values.
    /// @param _e The end iterator of the range of values.
    KDTreeLeaf(iterator_t _b, iterator_t _e) : KDTreeAbstractNode(_b, _e) {}

    /// @brief Perform a range on this leaf node.
    ///
    /// Performing a range search on a leaf node is rather simple, because we
    /// just need to check whether each of the elements in the node is actually
    /// inside the range.
    ///
    /// @param r The range to search for.
    /// @param f The mapping function to apply.
    virtual void rangeSearchMapDiscard(
        const range_t &r,
        std::function<void(const coordinate_t &, const Type &)> f)
        const override {
      // Determine whether the range completely covers the bounding box of this
      // leaf node. If it is, we can copy all values without having to check
      // for them being inside the range again.
      bool contained = r >= this->m_range;

      // Iterate over all the elements in this leaf node. This should be a
      // relatively small number (the LeafSize template parameter).
      for (iterator_t i = this->m_begin_it; i != this->m_end_it; ++i) {
        // We need to check whether the element is actually inside the range.
        // In case this node's bounding box is fully contained within the
        // range, we don't actually need to check this.
        if (contained || r.contains(i->first)) {
          f(i->first, i->second);
        }
      }
    }

    /// @brief Debugging string method for this (sub-)k-d tree.
    ///
    /// This prints information to stdout about the structure of the (sub-)tree
    /// defined by this node. Not designed for use in real code.
    ///
    /// @param i The amount of indentation to use.
    virtual std::string toString(std::size_t i) const override {
      std::stringstream out;

      // Again, we start out by printing some nice indentation.
      for (std::size_t j = 0; j < i; ++j) {
        out << " ";
      }

      // There isn't much to say about leaf nodes...
      out << "Leaf with " << this->size() << " objects" << std::endl;

      return out.str();
    }
  };

  /// @brief Vector containing all of the elements in this k-d tree, including
  /// the elements managed by the nodes inside of it.
  vector_t m_elems;

  /// @brief Pointer to the root node of this k-d tree.
  std::unique_ptr<KDTreeAbstractNode> m_root;
};
}  // namespace Acts
