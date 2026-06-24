#ifndef MCC2LM_SQL_HPP

#define MCC2LM_SQL_HPP

#include <functional>
#include <string>
#include <utility>
#include <sqlite3.h>
#include <mcc2lm/exceptions.hpp>

namespace mcc2lm
{
    void throw_sqlite_error(sqlite3 *db, const std::string &context)
    {
        throw mcc2lm::DatabaseException(context + ": " + sqlite3_errmsg(db));
    }

    class SqliteStatement
    {
        sqlite3_stmt *stmt = nullptr;

    public:
        SqliteStatement(sqlite3 *db, const std::string &query, const std::string &context)
        {
            if (sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr) != SQLITE_OK)
            {
                throw_sqlite_error(db, context);
            }
        }

        ~SqliteStatement()
        {
            if (stmt != nullptr)
            {
                sqlite3_finalize(stmt);
            }
        }

        SqliteStatement(const SqliteStatement &) = delete;
        SqliteStatement &operator=(const SqliteStatement &) = delete;

        sqlite3_stmt *get() const
        {
            return stmt;
        }
    };

    void bind_int(sqlite3 *db, sqlite3_stmt *stmt, int idx, int value, const std::string &context)
    {
        if (sqlite3_bind_int(stmt, idx, value) != SQLITE_OK)
        {
            throw_sqlite_error(db, context);
        }
    }

    void bind_text(sqlite3 *db, sqlite3_stmt *stmt, int idx, const std::string &value, const std::string &context)
    {
        if (sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
        {
            throw_sqlite_error(db, context);
        }
    }

    bool step_is_row_or_done(sqlite3 *db, sqlite3_stmt *stmt, const std::string &context)
    {
        const int result = sqlite3_step(stmt);
        if (result == SQLITE_ROW)
        {
            return true;
        }

        if (result == SQLITE_DONE)
        {
            return false;
        }

        throw_sqlite_error(db, context);
        return false;
    }

    void step_expect_done(sqlite3 *db, sqlite3_stmt *stmt, const std::string &context)
    {
        if (sqlite3_step(stmt) != SQLITE_DONE)
        {
            throw_sqlite_error(db, context);
        }
    }

    bool row_exists(
        sqlite3 *db,
        const std::string &query,
        const std::function<void(sqlite3_stmt *)> &bind_params,
        const std::string &context)
    {
        SqliteStatement stmt(db, query, context + " [prepare]");
        bind_params(stmt.get());
        return step_is_row_or_done(db, stmt.get(), context + " [step]");
    }

    std::pair<bool, std::string> optional_text_value(
        sqlite3 *db,
        const std::string &query,
        const std::function<void(sqlite3_stmt *)> &bind_params,
        int text_column_idx,
        const std::string &context)
    {
        SqliteStatement stmt(db, query, context + " [prepare]");
        bind_params(stmt.get());

        if (!step_is_row_or_done(db, stmt.get(), context + " [step]"))
        {
            return std::make_pair(false, "");
        }

        const unsigned char *raw_value = sqlite3_column_text(stmt.get(), text_column_idx);
        const std::string value = raw_value != nullptr ? reinterpret_cast<const char *>(raw_value) : "";
        return std::make_pair(true, value);
    }

    void execute_non_query(
        sqlite3 *db,
        const std::string &query,
        const std::function<void(sqlite3_stmt *)> &bind_params,
        const std::string &context)
    {
        SqliteStatement stmt(db, query, context + " [prepare]");
        bind_params(stmt.get());
        step_expect_done(db, stmt.get(), context + " [step]");
    }

    void ensure_mapping_row(
        sqlite3 *db,
        const std::string &exists_query,
        const std::string &insert_query,
        const std::function<void(sqlite3_stmt *)> &bind_keys,
        const std::string &context)
    {
        if (row_exists(db, exists_query, bind_keys, context + " [exists]"))
        {
            return;
        }

        execute_non_query(db, insert_query, bind_keys, context + " [insert]");
    }
}

#endif
