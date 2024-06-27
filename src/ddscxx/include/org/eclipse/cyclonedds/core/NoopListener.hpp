// Copyright(c) 2024 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#ifndef CYCLONEDDS_CORE_NOOPLISTENER_H_
#define CYCLONEDDS_CORE_NOOPLISTENER_H_

#include <memory>
#include <functional>
#include "dds/dds.h"

#include "dds/core/macros.hpp"

namespace org { namespace eclipse { namespace cyclonedds { namespace core {

extern OMG_DDS_API std::unique_ptr<dds_listener_t, std::function<void(dds_listener_t *)>> make_noop_listener();

} } } }

#endif /* CYCLONEDDS_CORE_NOOPLISTENER_H_ */
