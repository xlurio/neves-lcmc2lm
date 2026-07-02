#ifndef MCC2LM_SQL_HPP

#define MCC2LM_SQL_HPP

#include <chrono>
#include <functional>
#include <string>
#include <thread>
#include <utility>
#include <sqlite3.h>
#include <mcc2lm/exceptions.hpp>

namespace mcc2lm
{
#include <mcc2lm/impl/sql_impl_part1.inc>
#include <mcc2lm/impl/sql_impl_part2.inc>
}

#endif
