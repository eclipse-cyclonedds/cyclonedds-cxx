/*
 * Copyright(c) 2006 to 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#ifndef CYCLONEDDS_DDS_CORE_DETAIL_ARRAY_HPP_
#define CYCLONEDDS_DDS_CORE_DETAIL_ARRAY_HPP_

#include <dds/core/detail/macros.hpp>

/* Compiling explicitly w/ C++ 11 support */
#if defined(OSPL_USE_CXX11)
#  include <array>
#  define OSPL_CXX11_STD_MODULE ::std
/* Compiling to use Tech Report 1 headers */
#elif defined(OSPL_USE_TR1)
#  ifdef _MSC_VER
#    include <array>
#  else
#    include <tr1/array>
#  endif
#  define OSPL_CXX11_STD_MODULE ::std::tr1
/* Compiling with boost */
#elif defined(OSPL_USE_BOOST)
#  include <boost/array.hpp>
#  define OSPL_CXX11_STD_MODULE ::boost
#endif


namespace dds
{
namespace core
{
namespace detail
{
using OSPL_CXX11_STD_MODULE::array;
}
}
}

#endif /* CYCLONEDDS_DDS_CORE_DETAIL_ARRAY_HPP_ */
