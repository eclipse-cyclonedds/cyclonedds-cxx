#ifndef _HELLOWORLDDATA_H_
#define _HELLOWORLDDATA_H_

#include "dds/core/ddscore.hpp"

namespace HelloWorldData
{
    class Msg OSPL_DDS_FINAL
    {
    public:
        int32_t userID_;
        std::string message_;

    public:
        Msg() :
                userID_(0) {}

        explicit Msg(
            int32_t userID,
            const std::string& message) : 
                userID_(userID),
                message_(message) {}

        Msg(const Msg &_other) : 
                userID_(_other.userID_),
                message_(_other.message_) {}

#ifdef OSPL_DDS_CXX11
        Msg(Msg &&_other) : 
                userID_(::std::move(_other.userID_)),
                message_(::std::move(_other.message_)) {}

        Msg& operator=(Msg &&_other)
        {
            if (this != &_other) {
                userID_ = ::std::move(_other.userID_);
                message_ = ::std::move(_other.message_);
            }
            return *this;
        }
#endif

        Msg& operator=(const Msg &_other)
        {
            if (this != &_other) {
                userID_ = _other.userID_;
                message_ = _other.message_;
            }
            return *this;
        }

        bool operator==(const Msg& _other) const
        {
            return userID_ == _other.userID_ &&
                message_ == _other.message_;
        }

        bool operator!=(const Msg& _other) const
        {
            return !(*this == _other);
        }

        int32_t userID() const { return this->userID_; }
        int32_t& userID() { return this->userID_; }
        void userID(int32_t _val_) { this->userID_ = _val_; }
        const std::string& message() const { return this->message_; }
        std::string& message() { return this->message_; }
        void message(const std::string& _val_) { this->message_ = _val_; }
#ifdef OSPL_DDS_CXX11
        void message(std::string&& _val_) { this->message_ = _val_; }
#endif

    };

    size_t write_struct(const Msg& obj, void* data, size_t position);
    size_t write_size(const Msg& obj, size_t offset);
    size_t read_struct(Msg& obj, const void* data, size_t position);

}

#endif /* _HELLOWORLDDATA_H_ */
