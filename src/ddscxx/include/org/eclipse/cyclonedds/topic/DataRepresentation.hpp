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
#ifndef CYCLONEDDS_TOPIC_DATA_REPRESENTATION_HPP
#define CYCLONEDDS_TOPIC_DATA_REPRESENTATION_HPP

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace topic
{

typedef int16_t DataRepresentationId_t;

const DataRepresentationId_t XCDR_REPRESENTATION  = 0;
const DataRepresentationId_t XML_REPRESENTATION   = 0x001;
const DataRepresentationId_t OSPL_REPRESENTATION  = 0x400;
const DataRepresentationId_t GPB_REPRESENTATION   = 0x401;
const DataRepresentationId_t INVALID_REPRESENTATION = 0x7FFF;

}
}
}
}

#endif /* CYCLONEDDS_TOPIC_DATA_REPRESENTATION_HPP */
