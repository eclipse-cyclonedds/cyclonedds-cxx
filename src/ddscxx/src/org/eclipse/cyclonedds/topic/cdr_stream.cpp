/*
 * Copyright(c) 2020 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include <org/eclipse/cyclonedds/topic/cdr_stream.hpp>
#include <cstring>
#include <algorithm>

void cdr_stream::set_buffer(void* toset) {
  m_buffer = static_cast<char*>(toset);
  reset_position();
}

size_t cdr_stream::align(size_t newalignment, bool add_zeroes) {
  if (m_current_alignment == newalignment)
    return 0;

  m_current_alignment = std::min(newalignment, m_max_alignment);

  size_t tomove = (m_current_alignment - m_position % m_current_alignment) % m_current_alignment;
  if (tomove &&
    add_zeroes &&
    m_buffer)
    memset(get_cursor(), 0, tomove);

  m_position += tomove;

  return tomove;
}
