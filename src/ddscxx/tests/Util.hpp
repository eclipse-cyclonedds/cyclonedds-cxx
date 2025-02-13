// Copyright(c) 2021 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#ifndef _TEST_UTIL_H
#define _TEST_UTIL_H

#include "dds/dds.hpp"

#include <stdint.h>
#include <stddef.h>

/* Get unique g_topic name on each invocation. */
char *create_unique_topic_name (const char *prefix, char *name, size_t size);

/* I have no idea why gcc-10 and gcc-11 fail if this overload isn't available in the tests */
#if defined __GNUC__ && __GNUC__ < 12
inline std::ostream& operator << (std::ostream& os, const dds::core::TInstanceHandle<org::eclipse::cyclonedds::core::InstanceHandleDelegate> h)
{
  os << h.delegate();
  return os;
}
#endif

#endif /* _TEST_UTIL_H */
