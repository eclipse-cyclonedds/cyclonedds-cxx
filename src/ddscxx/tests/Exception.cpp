/*
 * Copyright(c) 2006 to 2018 ADLINK Technology Limited and others
 *
 * This program and the accompanying materials are made available under the
 * terms of the Eclipse Public License v. 2.0 which is available at
 * http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
 * v. 1.0 which is available at
 * http://www.eclipse.org/org/documents/edl-v10.php.
 *
 * SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause
 */
#include "dds/dds.hpp"
#include "dds/ddscxx/test.h"


DDSCXX_TEST(ddscxx_Exception, Error)
{
    std::string msg("Test dds::core::Error");
    dds::core::Error e_by_msg(msg);
    dds::core::Error e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, InvalidDataError)
{
    std::string msg("Test dds::core::InvalidDataError");
    dds::core::InvalidDataError e_by_msg(msg);
    dds::core::InvalidDataError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, PreconditionNotMetError)
{
    std::string msg("Test dds::core::PreconditionNotMetError");
    dds::core::PreconditionNotMetError e_by_msg(msg);
    dds::core::PreconditionNotMetError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, UnsupportedError)
{
    std::string msg("Test dds::core::UnsupportedError");
    dds::core::UnsupportedError e_by_msg(msg);
    dds::core::UnsupportedError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, NotEnabledError)
{
    std::string msg("Test dds::core::NotEnabledError");
    dds::core::NotEnabledError e_by_msg(msg);
    dds::core::NotEnabledError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, InconsistentPolicyError)
{
    std::string msg("Test dds::core::InconsistentPolicyError");
    dds::core::InconsistentPolicyError e_by_msg(msg);
    dds::core::InconsistentPolicyError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, ImmutablePolicyError)
{
    std::string msg("Test dds::core::ImmutablePolicyError");
    dds::core::ImmutablePolicyError e_by_msg(msg);
    dds::core::ImmutablePolicyError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, AlreadyClosedError)
{
    std::string msg("Test dds::core::AlreadyClosedError");
    dds::core::AlreadyClosedError e_by_msg(msg);
    dds::core::AlreadyClosedError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, IllegalOperationError)
{
    std::string msg("Test dds::core::IllegalOperationError");
    dds::core::IllegalOperationError e_by_msg(msg);
    dds::core::IllegalOperationError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, OutOfResourcesError)
{
    std::string msg("Test dds::core::OutOfResourcesError");
    dds::core::OutOfResourcesError e_by_msg(msg);
    dds::core::OutOfResourcesError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, TimeoutError)
{
    std::string msg("Test dds::core::TimeoutError");
    dds::core::TimeoutError e_by_msg(msg);
    dds::core::TimeoutError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, InvalidDowncastError)
{
    std::string msg("Test dds::core::InvalidDowncastError");
    dds::core::InvalidDowncastError e_by_msg(msg);
    dds::core::InvalidDowncastError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, NullReferenceError)
{
    std::string msg("Test dds::core::NullReferenceError");
    dds::core::NullReferenceError e_by_msg(msg);
    dds::core::NullReferenceError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}


DDSCXX_TEST(ddscxx_Exception, InvalidArgumentError)
{
    std::string msg("Test dds::core::InvalidArgumentError");
    dds::core::InvalidArgumentError e_by_msg(msg);
    dds::core::InvalidArgumentError e_by_cpy(e_by_msg);
    ASSERT_STREQ(e_by_msg.what(), msg.c_str());
    ASSERT_STREQ(e_by_cpy.what(), msg.c_str());
}




