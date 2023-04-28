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

#include <dds/core/Exception.hpp>
#include <sstream>
#include <org/eclipse/cyclonedds/core/ReportUtils.hpp>
#include <dds/ddsrt/retcode.h>

#include <cassert>
#include <string>
#include <map>

#define ISOCPP_REPORT_BUFLEN 1024

namespace org
{
namespace eclipse
{
namespace cyclonedds
{
namespace core
{
namespace utils
{

OMG_DDS_API
std::string&
pretty_function(
    std::string& s)
{
    /* Last part is erased before the first part to
     * be able to prettify template functions. */

    /* Erase last part of the function name. */
    size_t end = s.find_first_of("(");
    if (end > 0) {
        s.erase(end);
    }

    /* Erase first part of the function name. */
    size_t last_tab = s.find_last_of("\t");
    size_t tmpl_space = s.rfind(", ");
    size_t last_space;
    if (tmpl_space > 0) {
        last_space = s.find_last_of(" ", tmpl_space - 1);
    } else {
        last_space = s.find_last_of(" ");
    }
    if ((last_tab > 0) && (last_tab > last_space)) {
        s.erase(0, last_tab + 1);
    }
    if ((last_space > 0) && (last_space > last_tab)) {
        s.erase(0, last_space + 1);
    }

    return s;
}


OMG_DDS_API
std::string&
convert_classname(
    std::string& s)
{
    size_t index1, index2;

    s = pretty_function(s);

    index1 = s.rfind("<DELEGATE>");
    if (index1 > 0) {
        index2 = s.rfind("::T", index1);
        s.erase(index1, 10);
        s.erase(index2+2, 1);
    } else {
        index1 = s.rfind(", DELEGATE>");
        if (index1 > 0) {
            s.erase(index1, 10);
        } else {
            index1 = s.find("org::eclipse::cyclonedds");
            if (index1 == 0) {
                index1 = s.rfind("Delegate");
                if (index1 > 0) {
                    index2 = s.rfind("Delegate::", index1);
                    if (index2 > 0) {
                        s.erase(index1, 8);
                        s.erase(index2, 8);
                        s.replace(0, 16, "dds");
                    }
                }
            }
        }
    }

    return s;
}

class ExceptionFactory
{
public:
    ExceptionFactory() :
        signature(NULL),
        code(error_code),
        line(0) {}
    ~ExceptionFactory() {}

    void prepare(int32_t _code,
                 const char *_file,
                 int32_t _line,
                 const char *_signature,
                 const char *_descr)
    {
        assert (_file != NULL);
        assert (_signature != NULL);
        assert (_descr != NULL);

        this->code = _code;
        this->file = _file;
        this->line = _line;
        this->signature = _signature;
        this->description = _descr;

        /* Prettify originating method name for context and logs. */
        this->function = _signature;
        this->function = pretty_function(this->function);
        if (this->function.empty()) {
            this->function = _signature;
        }

        /* Prepare exception description and context. */
        prepare_context();
    }

    void throw_exception()
    {
        /* Throw proper exception type. */
        switch (this->code) {
            case error_code:
                throw dds::core::Error(this->message());
            case unsupported_error_code:
                throw dds::core::UnsupportedError(this->message());
            case invalid_argument_code:
                throw dds::core::InvalidArgumentError(this->message());
            case precondition_not_met_error_code:
                throw dds::core::PreconditionNotMetError(this->message());
            case out_of_resources_error_code:
                throw dds::core::OutOfResourcesError(this->message());
            case not_enabled_error_code:
                throw dds::core::NotEnabledError(this->message());
            case immutable_policy_error_code:
                throw dds::core::ImmutablePolicyError(this->message());
            case inconsistent_policy_error_code:
                throw dds::core::InconsistentPolicyError(this->message());
            case already_closed_error_code:
                throw dds::core::AlreadyClosedError(this->message());
            case timeout_error_code:
                throw dds::core::TimeoutError(this->message());
            case illegal_operation_error_code:
                throw dds::core::IllegalOperationError(this->message());
            case null_reference_error_code:
                throw dds::core::NullReferenceError(this->message());
            default:
                throw dds::core::Error("Unknown error");
        }
    }

    std::string message()
    {
        std::stringstream tmp;
        tmp << this->description << "\n" <<
               this->context;
        return tmp.str();
    }

private:

    void prepare_context()
    {
        /* Get and set context information. */
        const char *node = "UnknownNode";
        std::stringstream tmp;

        tmp << "===============================================================================\n" <<
               "Context     : " << this->function.c_str() << "\n" <<
               "Node        : " << node << "\n";
        this->context = tmp.str();
    }

    std::string description;
    std::string context;

    std::string function;
    std::string file;

    const char *signature;
    int32_t code;
    int32_t line;
};





}
}
}
}
}


void
org::eclipse::cyclonedds::core::utils::report(
    int32_t code,
    int32_t reportType,
    const char *file,
    int32_t line,
    const char *signature,
    const char *format,
    ...)
{
    (void)code;
    (void)reportType;
    (void)file;
    (void)line;
    (void)signature;
    (void)format;
#if 0
    char buffer[ISOCPP_REPORT_BUFLEN];
    const char *function;
    const char *retcode;
    os_size_t offset = 0;
    va_list args;

    assert (file != NULL);
    assert (signature != NULL);
    assert (format != NULL);

    /* Prepare error description. */
    retcode = dds_err_str (code);
    if (retcode != NULL) {
        offset = strlen(retcode);
        assert (offset <= ISOCPP_REPORT_BUFLEN);
        (void)memcpy(buffer, retcode, offset);
    }
    va_start(args, format);
    (void)os_vsnprintf(buffer + offset, sizeof(buffer) - offset, format, args);
    va_end(args);

    /* Prettify function name. */
    std::string s(signature);
    if (!pretty_function(s).empty()) {
        function = s.c_str();
    } else {
        function = signature;
    }

    /* Add this report to the logs. */
    os_report((os_reportType)reportType, function, file, line, code, "%s", buffer);
#endif
}

void
org::eclipse::cyclonedds::core::utils::throw_exception(
    int32_t code,
    const char *file,
    int32_t line,
    const char *signature,
    const char *format,
    ...)
{
    char description[ISOCPP_REPORT_BUFLEN];
    ExceptionFactory factory;
    va_list args;
    va_start(args, format);
    (void)vsnprintf(description, sizeof(description), format, args);
    va_end(args);
    factory.prepare(code, file, line, signature, description);
    factory.throw_exception();
}

void
org::eclipse::cyclonedds::core::utils::check_ddsc_result_and_throw_exception(
    dds_return_t code,
    const char *file,
    int32_t line,
    const char *signature,
    const char *format,
    ...)
{
    if (code < 0) {
        char desc[ISOCPP_REPORT_BUFLEN];
        char msg[ISOCPP_REPORT_BUFLEN - 10];
        ExceptionFactory factory;

        va_list args;
        va_start(args, format);
        (void)vsnprintf(msg, sizeof(msg), format, args);
        va_end(args);
        (void)snprintf(desc, sizeof(desc), "Error %s - %s", dds_strretcode(-code), msg);

        switch (dds_err_nr(code)) {
        case DDS_RETCODE_TIMEOUT:
            factory.prepare(ISOCPP_TIMEOUT_ERROR, file, line, signature, desc);
            break;
        case DDS_RETCODE_UNSUPPORTED:
            factory.prepare(ISOCPP_UNSUPPORTED_ERROR, file, line, signature, desc);
            break;
        case DDS_RETCODE_NOT_ALLOWED:
        case DDS_RETCODE_ILLEGAL_OPERATION:
        case DDS_RETCODE_NOT_ALLOWED_BY_SECURITY:
            factory.prepare(ISOCPP_ILLEGAL_OPERATION_ERROR, file, line, signature, desc);
            break;
        case DDS_RETCODE_PRECONDITION_NOT_MET:
            factory.prepare(ISOCPP_PRECONDITION_NOT_MET_ERROR, file, line, signature, desc);
            break;
        case DDS_RETCODE_IMMUTABLE_POLICY:
            factory.prepare(ISOCPP_IMMUTABLE_POLICY_ERROR, file, line, signature, desc);
            break;
        case DDS_RETCODE_INCONSISTENT_POLICY:
            factory.prepare(ISOCPP_INCONSISTENT_POLICY_ERROR, file, line, signature, desc);
            break;

        case DDS_RETCODE_BAD_PARAMETER:
            factory.prepare(ISOCPP_INVALID_ARGUMENT_ERROR, file, line, signature, desc);
            break;

        case DDS_RETCODE_NOT_ENOUGH_SPACE:
        case DDS_RETCODE_OUT_OF_RESOURCES:
            factory.prepare(ISOCPP_OUT_OF_RESOURCES_ERROR, file, line, signature, desc);
            break;

        case DDS_RETCODE_ALREADY_DELETED:
            factory.prepare(ISOCPP_ALREADY_CLOSED_ERROR, file, line, signature, desc);
            break;

        case DDS_RETCODE_NOT_ENABLED:
            factory.prepare(ISOCPP_NOT_ENABLED_ERROR, file, line, signature, desc);
            break;

        default:
            factory.prepare(ISOCPP_ERROR, file, line, signature, desc);
            break;
        }

        factory.throw_exception();
    }
}
