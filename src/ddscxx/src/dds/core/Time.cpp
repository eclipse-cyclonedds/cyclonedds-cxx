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

#include <org/eclipse/cyclonedds/core/TimeHelper.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <dds/core/Time.hpp>
#include <dds/core/Duration.hpp>
#include <dds/core/Exception.hpp>

#define MS 1000
#define MiS 1000000
#define NS 1000000000


const dds::core::Time
dds::core::Time::invalid()
{
    static const Time inv(-1, 0x7fffffff);

    return inv;
}

dds::core::Time::Time()
    :  sec_(0L),
       nsec_(0u)
{ }

dds::core::Time::Time(int64_t s, uint32_t ns)
{
    /* Use setter functions to validate values. */
    this->sec(s);
    this->nanosec(ns);
}

int64_t dds::core::Time::sec() const
{
    return sec_;
}

void dds::core::Time::sec(int64_t s)
{
    if(s < 0 && s != -1) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "dds::core::Time::sec out of bounds");
    } else {
        /** @internal @bug OSPL-2308 RTF Time-ish coercion issue
        @see http://jira.prismtech.com:8080/browse/OSPL-2308 */
        sec_ =  s;
    }
}

uint32_t dds::core::Time::nanosec() const
{
    return nsec_;
}

void dds::core::Time::nanosec(uint32_t ns)
{
    if((ns >= NS && ns != 0x7fffffff) || (sec() == -1 && ns != 0x7fffffff)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "dds::core::Time::nanosec out of bounds");
    } else {
        nsec_ = ns;
    }
}

const dds::core::Time dds::core::Time::from_microsecs(int64_t microseconds)
{
    return Time(microseconds / MiS, static_cast<uint32_t>((microseconds % MiS) * MS));
}

const dds::core::Time dds::core::Time::from_millisecs(int64_t milliseconds)
{
    return Time(milliseconds / MS, static_cast<uint32_t>((milliseconds % MS) * MiS));
}

const dds::core::Time dds::core::Time::from_secs(double seconds)
{
    int64_t int_secs =  static_cast<int64_t>(seconds);
    uint32_t nanos = static_cast<uint32_t>((seconds - static_cast<double>(int_secs)) * NS);
    return Time(int_secs, nanos);
}

int dds::core::Time::compare(const Time& that) const
{
    int ret;
    if(*this > that) {
        ret = 1;
    } else if(*this < that) {
        ret = -1;
    } else {
        ret = 0;
    }
    return ret;
}

bool
dds::core::Time::operator >(const Time& that) const
{
    return (sec_ > that.sec_) || ((sec_ == that.sec_) && (nsec_ > that.nsec_));
}

bool
dds::core::Time::operator >=(const Time& that) const
{
    return !(*this < that);
}

bool
dds::core::Time::operator !=(const Time& that) const
{
    return !(*this == that);
}

bool
dds::core::Time::operator ==(const Time& that) const
{
    return (sec_ == that.sec_) && (nsec_ == that.nsec_);
}

bool
dds::core::Time::operator <=(const Time& that) const
{
    return !(*this > that);
}

bool
dds::core::Time::operator <(const Time& that) const
{
    return (sec_ < that.sec_) || ((sec_ == that.sec_) && (nsec_ < that.nsec_));
}

dds::core::Time& dds::core::Time::operator +=(const Duration& a_ti)
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", " operator += time");
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(a_ti, "dds::core::Time", " operator += duration");
    this->sec_ += a_ti.sec();
    uint32_t dns = this->nsec_ + a_ti.nanosec();
    if(dns > NS) {
        this->sec_++;
        this->nsec_ = dns % NS;
    } else {
        nsec_ = dns;
    }
    return *this;
}

dds::core::Time& dds::core::Time::operator -=(const Duration& a_ti)
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", " operator += time");
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(a_ti, "dds::core::Time", " operator += duration");
    try
    {
        dds::core::Time tmp (this->sec_ - a_ti.sec());
        uint32_t dns = a_ti.nanosec();
        uint32_t tmpNS;
        if(nsec_ < dns)
        {
            tmp.sec(tmp.sec() - 1);
            tmpNS = nsec_ + NS - dns;
        }
        else
        {
            tmpNS = nsec_ - dns;
        }
        tmp.nanosec(tmpNS);
        org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", " operator += tmp");
        this->nanosec(tmp.nanosec());
        this->sec(tmp.sec());
    }
    catch (dds::core::Exception &)
    {
        throw;
    }
    catch(std::exception& e)
    {
        std::string message("dds::core::Time::operator -= IllegalOperationError");
        message += " Arithmetic operation resulted in a out of bounds";
        message += "\n";
        message += e.what();
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, message.c_str());
    }
    return *this;
}

int64_t dds::core::Time::to_millisecs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", "to_millisecs");
    return ((sec_ * MS) + (nsec_ / MiS));
}

int64_t dds::core::Time::to_microsecs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", "to_microsecs");
    return ((sec_ * MiS) + (nsec_ / MS));
}

double dds::core::Time::to_secs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Time>(*this, "dds::core::Time", "to_secs");
    return static_cast<double>(sec_) + (static_cast<double>(nsec_) / NS);
}

const dds::core::Time operator +(const dds::core::Time& lhs, const dds::core::Duration& rhs)
{
    return dds::core::Time(lhs.sec(), lhs.nanosec()) += rhs;
}

const dds::core::Time operator +(const dds::core::Duration& lhs, const dds::core::Time& rhs)
{
    return dds::core::Time(rhs.sec(), rhs.nanosec()) += lhs;
}

const dds::core::Time operator -(const dds::core::Time& lhs, const dds::core::Duration& rhs)
{
    return dds::core::Time(lhs.sec(), lhs.nanosec()) -= rhs;
}
