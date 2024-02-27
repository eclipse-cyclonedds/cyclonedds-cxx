// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 
/**
 * @file
 */

#ifndef CYCLONEDDS_UTIL_OSTREAM_OPERATORS_HPP_
#define CYCLONEDDS_UTIL_OSTREAM_OPERATORS_HPP_

#include "dds/features.hpp"

#include <array>
#include <vector>
#if DDSCXX_USE_BOOST
#include <boost/optional.hpp>
#define DDSCXX_STD_IMPL boost
#else
#include <optional>
#define DDSCXX_STD_IMPL std
#endif


namespace std
{

template <typename T, size_t N>
std::ostream& operator<<(std::ostream& os, const std::array<T, N>& rhs)
{
  os << "[";
  for (std::size_t i = 0; i < N; i++)
  {
    os << rhs[i];
    if (i < N - 1)
    {
      os << ", ";
    }
  }
  os << "]";
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const std::vector<T>& rhs)
{
  os << "[";
  for(size_t i=0; i<rhs.size(); i++)
  {
    if (i != 0)
    {
      os << ", ";
    }
    os << rhs[i];
  }
  os << "]";
  return os;
}

template<typename T>
std::ostream& operator<<(std::ostream& os, const DDSCXX_STD_IMPL::optional<T>& rhs)
{
  return rhs ? os << rhs.value() : os;
}

} //namespace std

#endif /* CYCLONEDDS_UTIL_OSTREAM_OPERATORS_HPP_ */
