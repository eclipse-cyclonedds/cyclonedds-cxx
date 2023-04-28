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

#include <dds/sub/status/detail/DataStateImpl.hpp>

// Implementation

namespace dds
{
namespace sub
{
namespace status
{

SampleState::SampleState() : MaskType() { }
SampleState::SampleState(uint32_t i) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (i)) { }
SampleState::SampleState(const SampleState& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }
SampleState::SampleState(const MaskType& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }

ViewState::ViewState() : MaskType() { }
ViewState::ViewState(uint32_t m) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (m)) { }
ViewState::ViewState(const ViewState& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }
ViewState::ViewState(const MaskType& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }

InstanceState::InstanceState(uint32_t m) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (m)) { }
InstanceState::InstanceState() : MaskType() { }
InstanceState::InstanceState(const InstanceState& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }
InstanceState::InstanceState(const MaskType& src) : MaskType(
    /// @internal @note MSVC bug: Problems constructing a bitset from an unsigned long in the VC RC
    /// @see http://connect.microsoft.com/VisualStudio/feedback/details/532897
#if _MSC_VER == 1600
        static_cast<int>
#endif
        (src.to_ulong())) { }

} /* namespace status */
} /* namespace sub */
} /* namespace dds */

// End of implementation
