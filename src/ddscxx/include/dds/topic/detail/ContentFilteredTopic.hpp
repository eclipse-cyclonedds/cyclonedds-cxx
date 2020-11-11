#ifndef OMG_DDS_TOPIC_DETAIL_CONTENTFILTEREDTOPIC_HPP_
#define OMG_DDS_TOPIC_DETAIL_CONTENTFILTEREDTOPIC_HPP_

/* Copyright 2010, Object Management Group, Inc.
 * Copyright 2010, PrismTech, Inc.
 * Copyright 2010, Real-Time Innovations, Inc.
 * All rights reserved.
 *
 * Licensed under the Apache License, Version 2.0 (the "License");
 * you may not use this file except in compliance with the License.
 * You may obtain a copy of the License at
 *
 *     http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS,
 * WITHOUT WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.
 * See the License for the specific language governing permissions and
 * limitations under the License.
 */

#include <string>
#include <vector>

#include <dds/core/detail/conformance.hpp>
#include <dds/core/types.hpp>
#include <dds/topic/Topic.hpp>
#include <dds/topic/Filter.hpp>
#include <org/eclipse/cyclonedds/topic/TopicDescriptionDelegate.hpp>
#include <org/eclipse/cyclonedds/core/ScopedLock.hpp>

#ifdef OMG_DDS_CONTENT_SUBSCRIPTION_SUPPORT

namespace dds {
namespace topic {
namespace detail {

class FunctorHolderBase
{
public:
    FunctorHolderBase() { };

    virtual ~FunctorHolderBase() { };

    virtual bool check_sample(const void *sample) = 0;

    static bool c99_check_sample(const void *sample, void *arg)
    {
        FunctorHolderBase *funcHolder = static_cast<FunctorHolderBase *>(arg);
        return funcHolder->check_sample(sample);
    }
};

template <typename FUN, typename T>
class FunctorHolder : public FunctorHolderBase
{
public:
    /* Remove const to be able to call non-const functors. */
    FunctorHolder(FUN functor) : myFunctor(functor)
    {
    }

    virtual ~FunctorHolder() { };

    bool check_sample(const void *sample)
    {
        org::eclipse::cyclonedds::topic::copyOutFunction copyOut;
        T cxxSample;

        copyOut = org::eclipse::cyclonedds::topic::TopicTraits<T>::getCopyOut();
        copyOut(sample, &cxxSample);
        return myFunctor(cxxSample);
    }

private:
    FUN myFunctor;
};

template <typename T>
class ContentFilteredTopic  :
    public virtual org::eclipse::cyclonedds::topic::TopicDescriptionDelegate,
    public virtual org::eclipse::cyclonedds::core::DDScObjectDelegate
{
public:
    ContentFilteredTopic(
        const dds::topic::Topic<T>& topic,
        const std::string& name,
        const dds::topic::Filter& filter)
        : org::eclipse::cyclonedds::topic::TopicDescriptionDelegate(topic.domain_participant(), name, topic.type_name()),
          org::eclipse::cyclonedds::core::DDScObjectDelegate(),
          myTopic(topic),
          myFilter(filter),
          myFunctor(nullptr)
    {
        topic.delegate()->incrNrDependents();
        this->myParticipant.delegate()->add_cfTopic(*this);
    }

    virtual ~ContentFilteredTopic()
    {
        if (!this->closed) {
            try {
                this->close();
            } catch (...) {
                /* Empty: the exception throw should have already traced an error. */
            }
        }
    }

    virtual void close()
    {
        org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

        myTopic.delegate()->decrNrDependents();

        // Remove the ContentFilteredTopic from the list of topics in its participant.
        this->myParticipant.delegate()->remove_cfTopic(*this);

        org::eclipse::cyclonedds::core::ObjectDelegate::close();
    }

    void
    init(org::eclipse::cyclonedds::core::ObjectDelegate::weak_ref_type weak_ref)
    {
        /* Set weak_ref before passing ourselves to other isocpp objects. */
        this->set_weak_ref(weak_ref);
        /* Register topic at participant. */
        this->myParticipant.delegate()->add_cfTopic(*this);
    }

private:
#if 0
    void validate_filter()
    {
        q_expr expr = NULL;
        uint32_t length;
        c_value *params;

        length = myFilter.parameters_length();
        if (length < 100) {
            expr = q_parse(myFilter.expression().c_str());
            if (!expr ) {
                ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR,
                        "filter_expression '%s' is invalid", myFilter.expression().c_str());
            }
        } else {
            ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR,
                    "Invalid number of filter_parameters '%d', maximum is 99", length);
        }

        u_topic uTopic = (u_topic)(myTopic.delegate()->get_user_handle());

        params = reader_parameters();
        if (!u_topicContentFilterValidate2(uTopic, expr, params)) {
            ISOCPP_THROW_EXCEPTION(ISOCPP_INVALID_ARGUMENT_ERROR,
                    "filter_expression '%s' is invalid.", myFilter.expression().c_str());
        }
        q_dispose(expr);
        os_free(params);
    }
#endif
public:
    std::string reader_expression() const
    {
        std::string rExpr;

        rExpr += "select * from ";
        rExpr += myTopic.name();
        rExpr += " where ";
        rExpr += myFilter.expression();
        return rExpr;
    }
#if 0
    c_value *reader_parameters() const
    {
        c_value *params = NULL;
        uint32_t n, length;
        org::eclipse::cyclonedds::topic::FilterDelegate::const_iterator paramIterator;

        length = myFilter.parameters_length();
        params = (c_value *)os_malloc(length * sizeof(struct c_value));
        for (n = 0, paramIterator = myFilter.begin(); n < length; n++, paramIterator++) {
            params[n] = c_stringValue(const_cast<char *>(paramIterator->c_str()));
        }
        return params;
    }
#endif
    /**
    *  @internal Accessor to return the topic filter.
    * @return The dds::topic::Filter in effect on this topic.
    */
    const dds::topic::Filter& filter() const
    {
        return myFilter;
    }

    /**
     *  @internal Sets the filter parameters for this content filtered topic.
     * @param begin The iterator holding the first string param
     * @param end The last item in the string iteration
     */
    template <typename FWIterator>
    void filter_parameters(const FWIterator& begin, const FWIterator& end)
    {
        ISOCPP_THROW_EXCEPTION(ISOCPP_UNSUPPORTED_ERROR, "Changing of Filter parameters is currently not supported.");
        myFilter.parameters(begin, end);
        //@todo validate_filter();
    }

    const dds::topic::Topic<T>& topic() const
    {
        return myTopic;
    }

    const std::string& filter_expression() const
    {
        return myFilter.expression();
    }

    const dds::core::StringSeq filter_parameters() const
    {
        return dds::core::StringSeq(myFilter.begin(), myFilter.end());
    }
#if 0
    dds::topic::TTopicDescription<TopicDescriptionDelegate> clone()
    {
        org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);

        typename dds::topic::ContentFilteredTopic<T, ContentFilteredTopic>::DELEGATE_REF_T ref(
                new ContentFilteredTopic<T>(this->myTopic, this->myTopicName, this->myFilter));
        ref->init(ref);

        return dds::topic::ContentFilteredTopic<T, ContentFilteredTopic>(ref);
    }
#endif

    template <typename Functor>
    void filter_function(Functor func)
    {
        /* Make a private copy of the topic so my filter doesn't bother the original topic. */
        dds_qos_t* ddsc_qos = myTopic.qos()->ddsc_qos();
        dds_entity_t cfTopic = dds_create_topic(
            myTopic.domain_participant().delegate()->get_ddsc_entity(),
            org::eclipse::cyclonedds::topic::TopicTraits<T>::getDescriptor(),
            myTopic.name().c_str(),
            ddsc_qos,
            NULL);
        dds_delete_qos(ddsc_qos);
        this->set_ddsc_entity(cfTopic);

        org::eclipse::cyclonedds::core::ScopedObjectLock scopedLock(*this);
        if (this->myFunctor)
        {
            delete this->myFunctor;
        }
        myFunctor = new FunctorHolder<Functor, T>(func);
        dds_set_topic_filter_and_arg(cfTopic, FunctorHolderBase::c99_check_sample, myFunctor);
    }

private:
    dds::topic::Topic<T> myTopic;
    dds::topic::Filter myFilter;
    FunctorHolderBase *myFunctor;
};

}
}
}

#endif /* OMG_DDS_CONTENT_SUBSCRIPTION_SUPPORT */

#endif /* OMG_DDS_TOPIC_DETAIL_CONTENTFILTEREDTOPIC_HPP_ */
