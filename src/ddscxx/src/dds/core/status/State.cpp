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

#include <dds/core/status/State.hpp>

namespace dds
{
namespace core
{
namespace status
{

SampleRejectedState::SampleRejectedState() : MaskType() { }

SampleRejectedState::SampleRejectedState(const SampleRejectedState& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }

SampleRejectedState::SampleRejectedState(const MaskType& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897 */
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }

SampleRejectedState::SampleRejectedState(uint32_t s)
    : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (s))
{ }

StatusMask::StatusMask() { }

StatusMask::StatusMask(uint32_t mask) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (mask)) { }

StatusMask::StatusMask(const StatusMask& other) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (other.to_ulong())) { }

StatusMask::~StatusMask() { }


}
}
} /* namespace dds / core / status*/
