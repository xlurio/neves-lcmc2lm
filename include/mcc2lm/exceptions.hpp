#ifndef MCC2LM_EXCEPTIONS_HPP

#define MCC2LM_EXCEPTIONS_HPP

namespace mcc2lm
{
    struct DatabaseException : public std::runtime_error
    {
        using std::runtime_error::runtime_error;
    };
}

#endif
