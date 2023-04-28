// Copyright(c) 2006 to 2020 ZettaScale Technology and others
//
// This program and the accompanying materials are made available under the
// terms of the Eclipse Public License v. 2.0 which is available at
// http://www.eclipse.org/legal/epl-2.0, or the Eclipse Distribution License
// v. 1.0 which is available at
// http://www.eclipse.org/org/documents/edl-v10.php.
//
// SPDX-License-Identifier: EPL-2.0 OR BSD-3-Clause

#include <org/eclipse/cyclonedds/domain/DomainWrap.hpp>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <org/eclipse/cyclonedds/domain/Domain.hpp>

org::eclipse::cyclonedds::domain::DomainWrap::DomainWrap(dds_domainid_t id, const std::string& config)
{
    this->ddsc_domain = -1;
    /*
     * If the config string is not empty, we have to create the domain
     * explicitly to get that config into the system.
     *
     * If not, don't do anything and let the system decide what config
     * to use.
     */
    if (!config.empty()) {
        ISOCPP_BOOL_CHECK_AND_THROW(id != org::eclipse::cyclonedds::domain::default_id(),
                                    ISOCPP_INVALID_ARGUMENT_ERROR,
                                    "When explicitly provide a config, a specific domain id has to be provided as well.");
        this->ddsc_domain = dds_create_domain(id, config.c_str());
        ISOCPP_DDSC_RESULT_CHECK_AND_THROW(this->ddsc_domain, "Failed to create domain explicitly.");
    }
}

org::eclipse::cyclonedds::domain::DomainWrap::DomainWrap(dds_domainid_t id, const struct ddsi_config& config)
{
    this->ddsc_domain = dds_create_domain_with_rawconfig(id, &config);
    ISOCPP_DDSC_RESULT_CHECK_AND_THROW(this->ddsc_domain, "Failed to create domain explicitly.");
}

org::eclipse::cyclonedds::domain::DomainWrap::~DomainWrap()
{
    if (this->ddsc_domain > 0) {
        /* The domain was explicitly created, which means we have
         * to explicitly delete it. */
        dds_delete(this->ddsc_domain);
    }
}
