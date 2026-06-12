/*
 * Copyright(c) 2026 ZettaScale Technology and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */

#include "RecursiveMutualSequence.hpp"
#include "RecursiveOptionalSequenceSelf.hpp"
#include "RecursiveOptionalTypedefSequenceSelf.hpp"

int main()
{
  optional_sequence_probe::optional_sequence_node optional_sequence_node;
  mutual_sequence_probe::a mutual_a;
  mutual_sequence_probe::b mutual_b;
  optional_typedef_sequence_probe::node optional_typedef_sequence_node;

  (void) optional_sequence_node;
  (void) mutual_a;
  (void) mutual_b;
  (void) optional_typedef_sequence_node;

  return 0;
}
