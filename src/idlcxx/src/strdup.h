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
#ifndef IDL_STRDUP_H
#define IDL_STRDUP_H

#include <string.h>

#if _WIN32
# define idl_strdup(s) _strdup(s)
#else
# define idl_strdup(s) strdup(s)
#endif

#endif /* IDL_STRDUP_H */
