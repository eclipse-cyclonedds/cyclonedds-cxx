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

#include <ostream>
#include <dds/core/Exception.hpp>

#define DEFINE_LE_EXCEPTION(EXP) \
    EXP::EXP (const std::string& what) : Exception(), std::logic_error(what) { } \
    EXP::EXP (const EXP& src) : Exception(), std::logic_error(src.what()) {} \
    EXP::~EXP () throw () { } \
    const char* EXP::what() const throw () { return this->std::logic_error::what(); }

#define DEFINE_RE_EXCEPTION(EXP) \
    EXP::EXP (const std::string& what) : Exception(), std::runtime_error(what) { } \
    EXP::EXP (const EXP& src) : Exception(), std::runtime_error(src.what()) {} \
    EXP::~EXP () throw () { } \
    const char* EXP::what() const throw () { return this->std::runtime_error::what(); }

namespace dds
{
namespace core
{

// --- Exception: ------------------------------------------------------------

Exception::Exception() { }

Exception::~Exception() throw() { }

DEFINE_LE_EXCEPTION(Error)
DEFINE_LE_EXCEPTION(InvalidDataError)
DEFINE_LE_EXCEPTION(PreconditionNotMetError)
DEFINE_LE_EXCEPTION(UnsupportedError)
DEFINE_LE_EXCEPTION(NotEnabledError)
DEFINE_LE_EXCEPTION(InconsistentPolicyError)
DEFINE_LE_EXCEPTION(ImmutablePolicyError)
DEFINE_LE_EXCEPTION(AlreadyClosedError)
DEFINE_LE_EXCEPTION(IllegalOperationError)

DEFINE_RE_EXCEPTION(OutOfResourcesError)
DEFINE_RE_EXCEPTION(TimeoutError)
DEFINE_RE_EXCEPTION(InvalidDowncastError)
DEFINE_RE_EXCEPTION(NullReferenceError)

InvalidArgumentError::InvalidArgumentError(const std::string& what) : Exception(), std::invalid_argument(what) { }
InvalidArgumentError::InvalidArgumentError(const InvalidArgumentError& src) : Exception(), std::invalid_argument(src.what()) {}
InvalidArgumentError::~InvalidArgumentError() throw() {}
const char* InvalidArgumentError::what() const throw()
{
    return this->std::invalid_argument::what();
}

}
}
