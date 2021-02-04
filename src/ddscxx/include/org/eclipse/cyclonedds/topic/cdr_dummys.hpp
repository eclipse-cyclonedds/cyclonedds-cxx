/*
 * Copyright(c) 2021 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef CDR_DUMMYS_HPP_
#define CDR_DUMMYS_HPP_

#include <string>
#include <vector>
#include <array>

template<size_t N>
class idl_string : public std::string {
public:
  idl_string() : std::string() {}
  idl_string(const std::string& in) : std::string(in) {}
  idl_string(const char* in) : std::string(in) {}
};

template<typename T, size_t N>
class idl_sequence : public std::vector<T> {
public:
  idl_sequence() : std::vector<T>() {}
  idl_sequence(const std::vector<T>& in) : std::vector<T>(in) {}
  template<typename U>
  idl_sequence(const U& in) : std::vector<T>() { for (const auto& e : in) this->emplace_back(e); }
};

template<typename T, size_t N>
class idl_array : public std::array<T, N> {
public:
  idl_array() : std::array<T, N>() {}
  idl_array(const std::array<T, N>& in) : std::array<T, N>(in) {}
  template<typename U>
  idl_array(const U& in) : std::array<T, N>() { for (size_t i = 0; i < in.size() && i < N; i++) this->at(i) = in[i]; }
};

#endif //CDR_DUMMYS_HPP_
