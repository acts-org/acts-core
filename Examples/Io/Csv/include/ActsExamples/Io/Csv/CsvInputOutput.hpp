
#pragma once

// The contents of this file are originally sourced from
// https://github.com/acts-project/dfelibs/blob/master/dfe/dfe_io_dsv.hpp
// and
// https://github.com/acts-project/dfelibs/blob/master/dfe/dfe_namedtuple.hpp
// and were published with the following license statement:

// SPDX-PackageName: "ACTS"
// Copyright 2015,2018-2020 Moritz Kiehn
// SPDX-FileCopyrightText: 2016 CERN
// SPDX-License-Identifier: MIT
// SPDX-License-Identifier: MPL-2.0

#include "Acts/Utilities/Concepts.hpp"
#include "Acts/Utilities/Helpers.hpp"

#include <algorithm>
#include <array>
#include <cassert>
#include <fstream>
#include <ostream>
#include <sstream>
#include <string>
#include <tuple>
#include <utility>
#include <vector>

/// Enable tuple-like access and conversion for selected class/struct members.
///
/// This allows access to the selected members via `.get<I>()` or `get<I>(...)`,
/// conversion to equivalent `std::tuple<...>` via implicit conversion or
/// explicitly via `.tuple()`,  and assignment from equivalent tuples.
/// The names can be accessed via `::names()`.
#define DFE_NAMEDTUPLE(name, ...)                                       \
  using Tuple = decltype(::std::make_tuple(__VA_ARGS__));               \
  static ::std::array<::std::string, ::std::tuple_size<Tuple>::value>   \
  names() {                                                             \
    return ::ActsExamples::detail_dfe::unstringify<                     \
        ::std::tuple_size<Tuple>::value>((#__VA_ARGS__));               \
  }                                                                     \
  template <typename... U>                                              \
  name& operator=(const ::std::tuple<U...>& other) {                    \
    ::std::tie(__VA_ARGS__) = other;                                    \
    return *this;                                                       \
  }                                                                     \
  template <typename... U>                                              \
  name& operator=(::std::tuple<U...>&& other) {                         \
    ::std::tie(__VA_ARGS__) = ::std::forward<std::tuple<U...>>(other);  \
    return *this;                                                       \
  }                                                                     \
  operator Tuple() const {                                              \
    return ::std::make_tuple(__VA_ARGS__);                              \
  }                                                                     \
  Tuple tuple() const {                                                 \
    return ::std::make_tuple(__VA_ARGS__);                              \
  }                                                                     \
  template <std::size_t I>                                              \
  constexpr ::std::tuple_element_t<I, Tuple>& get() {                   \
    return ::std::get<I>(std::tie(__VA_ARGS__));                        \
  }                                                                     \
  template <::std::size_t I>                                            \
  constexpr const ::std::tuple_element_t<I, Tuple>& get() const {       \
    return ::std::get<I>(std::tie(__VA_ARGS__));                        \
  }                                                                     \
  template <::std::size_t I>                                            \
  friend constexpr ::std::tuple_element_t<I, Tuple>& get(name& nt) {    \
    return nt.template get<I>();                                        \
  }                                                                     \
  template <::std::size_t I>                                            \
  friend constexpr const ::std::tuple_element_t<I, Tuple>& get(         \
      const name& nt) {                                                 \
    return nt.template get<I>();                                        \
  }                                                                     \
  friend inline ::std::ostream& operator<<(::std::ostream& os,          \
                                           const name& nt) {            \
    return ::ActsExamples::detail_dfe::print_tuple(                     \
        os, nt.names(), nt.tuple(),                                     \
        ::std::make_index_sequence<::std::tuple_size<Tuple>::value>{}); \
  }

namespace ActsExamples {
// implementation helpers
namespace detail_dfe {

// Reverse macro stringification.
//
// Splits a string of the form `a, b, c` into components a, b, and c.
template <std::size_t N>
constexpr std::array<std::string, N> unstringify(const char* str) {
  assert(str && "Input string must be non-null");

  std::array<std::string, N> out;

  for (std::size_t idx = 0; idx < N; ++idx) {
    // skip leading whitespace
    while ((*str != '\0') && (*str == ' ')) {
      ++str;
    }
    // find the next separator or end-of-string
    const char* sep = str;
    while ((*sep != '\0') && (*sep != ',')) {
      ++sep;
    }
    // store component w/o the separator
    out[idx].assign(str, sep - str);
    // we can quit as soon as we reached the end of the input
    if (*sep == '\0') {
      break;
    }
    // start search for next component after the separator
    str = ++sep;
  }
  // TODO handle inconsistent number of entries? can it occur in expected use?
  return out;
}

// modified from http://stackoverflow.com/a/6245777
template <typename Names, typename Values, std::size_t... I>
inline std::ostream& print_tuple(std::ostream& os, const Names& n,
                                 const Values& v,
                                 std::index_sequence<I...> /*seq*/) {
  // we want to execute some expression for every entry in the index pack. this
  // requires a construction that can take a variable number of arguments into
  // which we can unpack the indices. inside a function, constructing an
  // array with an initializer list will do the job, i.e. we will effectively
  // create the following statement
  //
  //     int x[] = {...};
  //
  // since we do not care about the actual values within the array, the
  // initializer list is cast twice: once to the array type and then to void.
  // this ignores the actual values and silences warnings about unused
  // variables. to get the correct initializer list syntax, the array type
  // must be typedef'd as a single type. what we get is
  //
  //     using Vacuum = int[];
  //     (void)Vacuum{...};
  //
  // in order for this to work, the expression that we want to instantiate
  // needs to evaluate to the element type of the array (here: `int`). this can
  // be done with the comma operator (yep, `,` is a weird but helpful operator)
  // for arbitrary expressions. `(<expr1>, <expr2>)` executes both expressions
  // but evaluates only to the return value of the second expression. thus,
  // `(<expr>, 0)` executes `<expr>` but always evaluates to an integer of value
  // zero. if <expr> uses the index pack variable `I` in the following setup
  //
  //     (void)Vacuum{(<expr>, 0)...};
  //
  // it is instantiatied for each element within the pack (with appropriate ,
  // placements). thus, effectively looping over every entry in the pack and
  // calling <expr> for each (here: printing to os);
  using std::get;
  using Vacuum = int[];
  (void)Vacuum{
      (os << ((0 < I) ? " " : "") << get<I>(n) << "=" << get<I>(v), 0)...};
  return os;
}

/// Write arbitrary data as delimiter-separated values into a text file.
template <char Delimiter>
class DsvWriter {
 public:
  DsvWriter() = delete;
  DsvWriter(const DsvWriter&) = delete;
  DsvWriter(DsvWriter&&) = default;
  ~DsvWriter() = default;
  DsvWriter& operator=(const DsvWriter&) = delete;
  DsvWriter& operator=(DsvWriter&&) = default;

  /// Create a file at the given path. Overwrites existing data.
  ///
  /// \param columns    Column names, fixes the number of columns for the file
  /// \param path       Path to the output file
  /// \param precision  Output floating point precision
  DsvWriter(const std::vector<std::string>& columns, const std::string& path,
            int precision = std::numeric_limits<double>::max_digits10);

  /// Append arguments as a new row to the file.
  ///
  /// Each argument corresponds to one column. The writer ensures that the
  /// number of columns written match the number of columns that were specified
  /// during construction.
  ///
  /// \note `std::vector` arguments are automatically unpacked and each entry
  ///       is written as a separate column.
  template <typename Arg0, typename... Args>
  void append(Arg0&& arg0, Args&&... args);

 private:
  std::ofstream m_file;
  std::size_t m_num_columns;

  template <typename T>
  unsigned write(T&& x, std::ostream& os)
    requires(Acts::Concepts::arithmetic<std::decay_t<T>> ||
             std::convertible_to<T, std::string>);
  template <typename T, typename Allocator>
  static unsigned write(const std::vector<T, Allocator>& xs, std::ostream& os);
};

/// Read arbitrary data as delimiter-separated values from a text file.
template <char Delimiter>
class DsvReader {
 public:
  DsvReader() = delete;
  DsvReader(const DsvReader&) = delete;
  DsvReader(DsvReader&&) = default;
  ~DsvReader() = default;
  DsvReader& operator=(const DsvReader&) = delete;
  DsvReader& operator=(DsvReader&&) = default;

  /// Open a file at the given path.
  ///
  /// \param path Path to the input file
  explicit DsvReader(const std::string& path);

  /// Read the next line from the file.
  ///
  /// \returns true   if the line was successfully read
  /// \returns false  if no more lines are available
  bool read(std::vector<std::string>& columns);

  /// Return the number of lines read so far.
  std::size_t num_lines() const { return m_num_lines; }

 private:
  std::ifstream m_file;
  std::string m_line;
  std::size_t m_num_lines = 0;
};

/// Write records as delimiter-separated values into a text file.
template <char Delimiter, typename NamedTuple>
class NamedTupleDsvWriter {
 public:
  NamedTupleDsvWriter() = delete;
  NamedTupleDsvWriter(const NamedTupleDsvWriter&) = delete;
  NamedTupleDsvWriter(NamedTupleDsvWriter&&) = default;
  ~NamedTupleDsvWriter() = default;
  NamedTupleDsvWriter& operator=(const NamedTupleDsvWriter&) = delete;
  NamedTupleDsvWriter& operator=(NamedTupleDsvWriter&&) = default;

  /// Create a file at the given path. Overwrites existing data.
  ///
  /// \param path       Path to the output file
  /// \param precision  Output floating point precision
  NamedTupleDsvWriter(const std::string& path,
                      int precision = std::numeric_limits<double>::max_digits10)
      : m_writer(colum_names(), path, precision) {}

  /// Append a record to the file.
  void append(const NamedTuple& record) {
    append_impl(record, std::make_index_sequence<
                            std::tuple_size_v<typename NamedTuple::Tuple>>{});
  }

 private:
  DsvWriter<Delimiter> m_writer;

  static std::vector<std::string> colum_names() {
    const auto& from_record = NamedTuple::names();
    return {from_record.begin(), from_record.end()};
  }
  template <std::size_t... I>
  void append_impl(const NamedTuple& values,
                   std::index_sequence<I...> /*seq*/) {
    using std::get;
    m_writer.append(get<I>(values)...);
  }
};

// string conversion helper functions

template <typename T>
static void parse(const std::string& str, T& value) {
  // TODO use somtehing w/ lower overhead then stringstream e.g. std::from_chars
  std::istringstream is(str);
  is >> value;
}

/// Read records as delimiter-separated values from a text file.
///
/// The reader is strict about its input format to avoid ambiguities. If
/// header verification is disabled, the first line will be skipped and each
/// line must contain exactly as many columns as there are tuple members in
/// exactly the same order. If header verification is enabled, the first line
/// is interpreted as the header. Names in the header must match exactly to
/// the tuple members but can be in arbitrary order. The file can contain
/// extra columns that are not tuple members. Each following row must have
/// exactly the same number of columns as the header.
template <char Delimiter, typename NamedTuple>
class NamedTupleDsvReader {
 public:
  NamedTupleDsvReader() = delete;
  NamedTupleDsvReader(const NamedTupleDsvReader&) = delete;
  NamedTupleDsvReader(NamedTupleDsvReader&&) = default;
  ~NamedTupleDsvReader() = default;
  NamedTupleDsvReader& operator=(const NamedTupleDsvReader&) = delete;
  NamedTupleDsvReader& operator=(NamedTupleDsvReader&&) = default;

  /// Open a file at the given path.
  ///
  /// \param path              Path to the input file
  /// \param optional_columns  Record columns that can be missing in the file
  /// \param verify_header     true to check header column names, false to skip
  ///
  /// The set of optional columns must match names in the record. When allowing
  /// optional columns, header verification must be set to true.
  NamedTupleDsvReader(const std::string& path,
                      const std::vector<std::string>& optional_columns = {},
                      bool verify_header = true);

  /// Read the next record from the file.
  ///
  /// Extra columns in the file will be ignored. Elements of the record that
  /// correspond to missing, optional columns will not be set and retain
  /// their value.
  ///
  /// \returns true   if a record was successfully read
  /// \returns false  if no more records are available
  bool read(NamedTuple& record);

  /// Read the next record and any extra columns from the file.
  ///
  /// \returns true   if a record was successfully read
  /// \returns false  if no more records are available
  template <typename T>
  bool read(NamedTuple& record, std::vector<T>& extra);

  /// Return the number of additional columns that are not part of the tuple.
  std::size_t num_extra_columns() const { return m_extra_columns.size(); }
  /// Return the number of records read so far.
  std::size_t num_records() const { return m_reader.num_lines() - 1u; }

 private:
  // the equivalent std::tuple-like type
  using Tuple = typename NamedTuple::Tuple;

  DsvReader<Delimiter> m_reader;
  std::vector<std::string> m_columns;
  // #columns is fixed to a reasonable value after reading the header
  std::size_t m_num_columns = SIZE_MAX;
  // map tuple index to column index in the file, SIZE_MAX for missing elements
  std::array<std::size_t, std::tuple_size<Tuple>::value> m_tuple_column_map;
  // column indices that do not map to a tuple items
  std::vector<std::size_t> m_extra_columns;

  void use_default_columns();
  void parse_header(const std::vector<std::string>& optional_columns);
  template <std::size_t... I>
  void parse_record(NamedTuple& record,
                    std::index_sequence<I...> /*seq*/) const {
    // see namedtuple_impl::print_tuple for explanation
    // allow different column ordering on file and optional columns
    using Vacuum = int[];
    (void)Vacuum{(parse_element<I>(record), 0)...};
  }
  template <std::size_t I>
  void parse_element(NamedTuple& record) const {
    using std::get;
    if (m_tuple_column_map[I] != SIZE_MAX) {
      parse(m_columns[m_tuple_column_map[I]], get<I>(record));
    }
  }
};

// implementation writer

template <char Delimiter>
inline DsvWriter<Delimiter>::DsvWriter(const std::vector<std::string>& columns,
                                       const std::string& path, int precision)
    : m_file(path,
             std::ios_base::binary | std::ios_base::out | std::ios_base::trunc),
      m_num_columns(columns.size()) {
  if (!m_file.is_open() || m_file.fail()) {
    throw std::runtime_error("Could not open file '" + path + "'");
  }
  m_file.precision(precision);
  if (m_num_columns == 0) {
    throw std::invalid_argument("No columns were specified");
  }
  // write column names as header row
  append(columns);
}

template <char Delimiter>
template <typename Arg0, typename... Args>
inline void DsvWriter<Delimiter>::append(Arg0&& arg0, Args&&... args) {
  // we can only check how many columns were written after they have been
  // written. write to temporary first to prevent bad data on file.
  std::stringstream line;
  // ensure consistent formatting
  line.precision(m_file.precision());
  unsigned written_columns[] = {
      // write the first item without a delimiter and store columns written
      write(std::forward<Arg0>(arg0), line),
      // for all other items, write the delimiter followed by the item itself
      // (<expr1>, <expr2>) use the comma operator (yep, ',' in c++ is a weird
      // but helpful operator) to execute both expression and return the return
      // value of the last one, i.e. here that's the number of columns written.
      // the ... pack expansion creates this expression for all arguments
      (line << Delimiter, write(std::forward<Args>(args), line))...,
  };
  line << '\n';
  // validate that the total number of written columns matches the specs.
  unsigned total_columns = 0;
  for (auto nc : written_columns) {
    total_columns += nc;
  }
  if (total_columns < m_num_columns) {
    throw std::invalid_argument("Not enough columns");
  }
  if (m_num_columns < total_columns) {
    throw std::invalid_argument("Too many columns");
  }
  // write the line to disk and check that it actually happened
  m_file << line.rdbuf();
  if (!m_file.good()) {
    throw std::runtime_error("Could not write data to file");
  }
}

template <char Delimiter>
template <typename T>
inline unsigned DsvWriter<Delimiter>::write(T&& x, std::ostream& os)
  requires(Acts::Concepts::arithmetic<std::decay_t<T>> ||
           std::convertible_to<T, std::string>)
{
  os << x;
  return 1u;
}

template <char Delimiter>
template <typename T, typename Allocator>
inline unsigned DsvWriter<Delimiter>::write(const std::vector<T, Allocator>& xs,
                                            std::ostream& os) {
  unsigned n = 0;
  for (const auto& x : xs) {
    if (0 < n) {
      os << Delimiter;
    }
    os << x;
    n += 1;
  }
  return n;
}

// implementation reader

template <char Delimiter>
inline DsvReader<Delimiter>::DsvReader(const std::string& path)
    : m_file(path, std::ios_base::binary | std::ios_base::in) {
  if (!m_file.is_open() || m_file.fail()) {
    throw std::runtime_error("Could not open file '" + path + "'");
  }
}

template <char Delimiter>
inline bool DsvReader<Delimiter>::read(std::vector<std::string>& columns) {
  // read the next line and check for both end-of-file and errors
  std::getline(m_file, m_line);
  if (m_file.eof()) {
    return false;
  }
  if (m_file.fail()) {
    throw std::runtime_error(std::string("Could not read line ") +
                             std::to_string(m_num_lines));
  }
  m_num_lines += 1;

  // split the line into columns
  columns.clear();
  for (std::string::size_type pos = 0; pos < m_line.size();) {
    auto del = m_line.find_first_of(Delimiter, pos);
    if (del == std::string::npos) {
      // reached the end of the line; also determines the last column
      columns.emplace_back(m_line, pos);
      break;
    } else {
      columns.emplace_back(m_line, pos, del - pos);
      // start next column search after the delimiter
      pos = del + 1;
    }
  }
  return true;
}

// implementation named tuple reader

template <char Delimiter, typename NamedTuple>
inline NamedTupleDsvReader<Delimiter, NamedTuple>::NamedTupleDsvReader(
    const std::string& path, const std::vector<std::string>& optional_columns,
    bool verify_header)
    : m_reader(path) {
  // optional columns only work if we verify the header
  if ((!optional_columns.empty()) && (!verify_header)) {
    throw std::runtime_error(
        "Optional columns can not be used without header verification");
  }
  // first line is always the header
  if (!m_reader.read(m_columns)) {
    throw std::runtime_error("Could not read header from '" + path + "'");
  }
  if (verify_header) {
    parse_header(optional_columns);
  } else {
    use_default_columns();
  }
}

template <char Delimiter, typename NamedTuple>
inline bool NamedTupleDsvReader<Delimiter, NamedTuple>::read(
    NamedTuple& record) {
  if (!m_reader.read(m_columns)) {
    return false;
  }
  // check for consistent entries per-line
  if (m_columns.size() < m_num_columns) {
    throw std::runtime_error("Too few columns in line " +
                             std::to_string(m_reader.num_lines()));
  }
  if (m_num_columns < m_columns.size()) {
    throw std::runtime_error("Too many columns in line " +
                             std::to_string(m_reader.num_lines()));
  }
  // convert to tuple
  parse_record(record, std::make_index_sequence<std::tuple_size_v<Tuple>>{});
  return true;
}

template <char Delimiter, typename NamedTuple>
template <typename T>
inline bool NamedTupleDsvReader<Delimiter, NamedTuple>::read(
    NamedTuple& record, std::vector<T>& extra) {
  // parse columns belonging to the regular record
  if (!read(record)) {
    return false;
  }
  // parse extra columns
  extra.resize(m_extra_columns.size());
  for (std::size_t i = 0; i < m_extra_columns.size(); ++i) {
    parse(m_columns[m_extra_columns[i]], extra[i]);
  }
  return true;
}

template <char Delimiter, typename NamedTuple>
inline void NamedTupleDsvReader<Delimiter, NamedTuple>::use_default_columns() {
  // assume row content is identical in content and order to the tuple
  m_num_columns = std::tuple_size_v<Tuple>;
  for (std::size_t i = 0; i < m_tuple_column_map.size(); ++i) {
    m_tuple_column_map[i] = i;
  }
  // no extra columns by construction
  m_extra_columns.clear();
}

template <char Delimiter, typename NamedTuple>
inline void NamedTupleDsvReader<Delimiter, NamedTuple>::parse_header(
    const std::vector<std::string>& optional_columns) {
  const auto& names = NamedTuple::names();

  // the number of header columns fixes the expected number of data columns
  m_num_columns = m_columns.size();

  // check that all non-optional columns are available
  for (const auto& name : names) {
    // no need to for availability if the column is optional
    if (Acts::rangeContainsValue(optional_columns, name)) {
      continue;
    }
    // missing, non-optional column mean we can not continue
    if (!Acts::rangeContainsValue(m_columns, name)) {
      throw std::runtime_error("Missing header column '" + name + "'");
    }
  }

  // ensure missing columns are correctly marked as such
  m_tuple_column_map.fill(SIZE_MAX);

  // determine column-tuple mapping and extra column indices
  m_extra_columns.clear();
  for (std::size_t i = 0; i < m_columns.size(); ++i) {
    // find the position of the column in the tuple.
    auto it = std::ranges::find(names, m_columns[i]);
    if (it != names.end()) {
      // establish mapping between column and tuple item position
      m_tuple_column_map[std::distance(names.begin(), it)] = i;
    } else {
      // record non-tuple columns
      m_extra_columns.push_back(i);
    }
  }
}

#undef _UNUSED

}  // namespace detail_dfe

/// Write arbitrary data as comma-separated values into as text file.
using CsvWriter = detail_dfe::DsvWriter<','>;

/// Write arbitrary data as tab-separated values into as text file.
using TsvWriter = detail_dfe::DsvWriter<'\t'>;

/// Write tuple-like records as comma-separated values into a text file.
template <typename T>
using NamedTupleCsvWriter = detail_dfe::NamedTupleDsvWriter<',', T>;

/// Read tuple-like records from a comma-separated file.
template <typename T>
using NamedTupleCsvReader = detail_dfe::NamedTupleDsvReader<',', T>;

/// Write tuple-like records as tab-separated values into a text file.
template <typename T>
using NamedTupleTsvWriter = detail_dfe::NamedTupleDsvWriter<'\t', T>;

/// Read tuple-like records from a tab-separated file.
template <typename T>
using NamedTupleTsvReader = detail_dfe::NamedTupleDsvReader<'\t', T>;

}  // namespace ActsExamples
