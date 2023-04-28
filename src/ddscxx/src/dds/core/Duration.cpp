// Copyright(c) 2006 to 2021 ZettaScale Technology and others
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

#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/core/TimeHelper.hpp>
#include <dds/core/Exception.hpp>
#include <dds/core/Time.hpp>
#include <dds/core/Duration.hpp>

#define MS 1000
#define MiS 1000000
#define NS 1000000000


dds::core::Duration::Duration()
    :  sec_(0),
       nsec_(0)
{ }

dds::core::Duration::Duration(int32_t s, uint32_t ns)
{
    /* Use setter functions to validate values. */
    this->sec(s);
    this->nanosec(ns);
}

#if __cplusplus >= 199711L
dds::core::Duration::Duration(int64_t s, uint32_t ns)
{
    /* Use setter functions to validate values. */
    this->sec(s);
    this->nanosec(ns);
}
#endif

int64_t dds::core::Duration::sec() const
{
    return sec_;
}

void dds::core::Duration::sec(int64_t s)
{
    if(s < 0) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "dds::core::Duration::sec out of bounds");
    } else {
        /** @internal @bug OSPL-2308 RTF Time-ish coercion issue
        @see http://jira.prismtech.com:8080/browse/OSPL-2308 */
        sec_ = static_cast<int32_t>(s);
    }
}

uint32_t dds::core::Duration::nanosec() const
{
    return nsec_;
}

void dds::core::Duration::nanosec(uint32_t ns)
{
    if((ns >= NS) && (ns != 0x7fffffff)) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "dds::core::Duration::nanosec out of bounds");
    } else {
        nsec_ = ns;
    }
}

const dds::core::Duration dds::core::Duration::from_microsecs(int64_t microseconds)
{
    return Duration(microseconds / MiS, static_cast<uint32_t>((microseconds % MiS) * MS));
}

const dds::core::Duration dds::core::Duration::from_millisecs(int64_t milliseconds)
{
    return Duration(milliseconds / MS, static_cast<uint32_t>((milliseconds % MS) * MiS));
}

const dds::core::Duration dds::core::Duration::from_secs(double seconds)
{
    int64_t int_secs =  static_cast<int64_t>(seconds);
    uint32_t nanos = static_cast<uint32_t>((seconds - static_cast<double>(int_secs)) * NS);
    return Duration(int_secs, nanos);
}

int dds::core::Duration::compare(const Duration& that) const
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

int64_t dds::core::Duration::to_millisecs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", "to_millisecs");
    return (static_cast<int64_t>(sec_) * MS) + (nsec_ / MiS);
}

int64_t dds::core::Duration::to_microsecs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", "to_microsecs");
    return (static_cast<int64_t>(sec_) * MiS) + (nsec_ / MS);
}


double dds::core::Duration::to_secs() const
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", "to_secs");
    return static_cast<double>(sec_) + (static_cast<double>(nsec_) / NS);
}

bool
dds::core::Duration::operator >(const Duration& that) const
{
    return (sec_ > that.sec_) || ((sec_ == that.sec_) && (nsec_ > that.nsec_));
}

bool
dds::core::Duration::operator >=(const Duration& that) const
{
    return !(*this < that);
}

bool
dds::core::Duration::operator !=(const Duration& that) const
{
    return !(*this == that);
}

bool
dds::core::Duration::operator ==(const Duration& that) const
{
    return (sec_ == that.sec_) && (nsec_ == that.nsec_);
}

bool
dds::core::Duration::operator <=(const Duration& that) const
{
    return !(*this > that);
}

bool
dds::core::Duration::operator <(const Duration& that) const
{
    return (sec_ < that.sec_) || ((sec_ == that.sec_) && (nsec_ < that.nsec_));
}

dds::core::Duration&
dds::core::Duration::operator +=(const Duration& a_ti)
{
  org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", "operator += this");
  org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(a_ti, "dds::core::Duration", "operator += a_ti");
  sec_ += static_cast<int32_t>(a_ti.sec());
  uint32_t dns = nsec_ + a_ti.nanosec();
  if (dns > NS)
  {
    sec_++;
    nsec_ = dns % NS;
  }
  else
  {
    nsec_ = dns;
  }
  return *this;
}

dds::core::Duration&
dds::core::Duration::operator -=(const Duration& a_ti)
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", "operator -= this");
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(a_ti, "dds::core::Duration", "operator -= a_ti");
    try {
        dds::core::Duration tmp(sec_ - a_ti.sec());
        uint32_t dns = a_ti.nanosec();
        uint32_t tmpNS;
        if(nsec_ < dns) {
            tmp.sec(tmp.sec() - 1);
            tmpNS = nsec_ + NS - dns;
        } else {
            tmpNS = nsec_ - dns;
        }
        tmp.nanosec(tmpNS);
        org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(tmp, "dds::core::Duration", "operator -= tmp");
        this->nanosec(tmp.nanosec());
        this->sec(tmp.sec());
    } catch (dds::core::Exception &) {
            throw;
    } catch(std::exception& e) {
        std::string message("dds::core::Duration::operator -= IllegalOperationError");
        message += " Arithmetic operation resulted in a out of bounds";
        message += "\n";
        message += e.what();
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, message.c_str());
    }

    return *this;
}

const dds::core::Duration dds::core::Duration::operator +(const Duration& other) const
{
    return(Duration(sec_, nsec_) += other);
}

const dds::core::Duration dds::core::Duration::operator -(const Duration& other) const
{
    return (Duration(sec_, nsec_) -= other);
}

dds::core::Duration&
dds::core::Duration::operator *=(uint64_t factor)
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(*this, "dds::core::Duration", " operator *=");
    this->sec(static_cast<int64_t>(this->sec_) * static_cast<int64_t>(factor));
    uint64_t ns = this->nanosec() * factor;
    if(ns  > NS) {
        this->sec(static_cast<int64_t>(this->sec_) + static_cast<int64_t>(ns) / static_cast<int64_t>(NS));
        /* cast below is safe because ns % NS is always < NS which is 32 bit */
        this->nanosec(static_cast<uint32_t>(ns % NS));
    } else {
        /* cast below is safe necause ns < NS in this clause */
        this->nanosec(static_cast<uint32_t>(ns));
    }
    return *this;
}

const dds::core::Duration dds::core::operator *(uint64_t lhs, const dds::core::Duration& rhs)
{
    return dds::core::Duration(rhs.sec(), rhs.nanosec()) *= lhs;
}

const dds::core::Duration dds::core::operator *(const dds::core::Duration& lhs, uint64_t rhs)
{
    return dds::core::Duration(lhs.sec(), lhs.nanosec()) *= rhs;
}

const dds::core::Duration dds::core::operator /(const dds::core::Duration& rhs, uint64_t lhs)
{
    org::eclipse::cyclonedds::core::timehelper::validate<dds::core::Duration>(rhs, "dds::core::Duration", " operator /");
    if (lhs == 0) {
        ISOCPP_THROW_EXCEPTION(ISOCPP_ERROR, "Divide by zero");
    }
    /* This will round up from nanoseconds to microseconds. Good enough for now. */
    return dds::core::Duration::from_microsecs(rhs.to_microsecs() / static_cast<int64_t>(lhs));
}

const dds::core::Duration
dds::core::Duration::infinite()
{
    static Duration d(static_cast<int64_t>(0x7fffffff), static_cast<uint32_t>(0x7fffffff));
    return d;
}

const dds::core::Duration
dds::core::Duration::zero()
{
    static Duration d(static_cast<int64_t>(0), static_cast<uint32_t>(0));
    return d;
}
