inline bool is_sqlite_lock_contention(int result)
{
    return result == SQLITE_BUSY || result == SQLITE_LOCKED;
}

inline int lock_retry_delay_ms(int attempt)
{
    const int base_delay_ms = 10;
    const int max_delay_ms = 250;
    const int scaled = base_delay_ms << attempt;
    return scaled < max_delay_ms ? scaled : max_delay_ms;
}

template <typename Func>
int call_with_sqlite_lock_retry(Func &&callable)
{
    constexpr int max_attempts = 8;
    int last_result = SQLITE_OK;

    for (int attempt = 0; attempt < max_attempts; ++attempt)
    {
        last_result = callable();
        if (!is_sqlite_lock_contention(last_result))
        {
            return last_result;
        }

        if (attempt + 1 < max_attempts)
        {
            std::this_thread::sleep_for(std::chrono::milliseconds(lock_retry_delay_ms(attempt)));
        }
    }

    return last_result;
}

inline void throw_sqlite_error(sqlite3 *db, const std::string &context)
{
    throw mcc2lm::DatabaseException(context + ": " + sqlite3_errmsg(db));
}

inline void execute_sqlite_exec(sqlite3 *db, const std::string &query, const std::string &context)
{
    const int result = call_with_sqlite_lock_retry(
        [&]()
        {
            return sqlite3_exec(db, query.c_str(), nullptr, nullptr, nullptr);
        });

    if (result != SQLITE_OK)
    {
        throw_sqlite_error(db, context);
    }
}

class SqliteStatement
{
    sqlite3_stmt *stmt = nullptr;

public:
    SqliteStatement(sqlite3 *db, const std::string &query, const std::string &context)
    {
        const int result = call_with_sqlite_lock_retry(
            [&]()
            {
                return sqlite3_prepare_v2(db, query.c_str(), -1, &stmt, nullptr);
            });

        if (result != SQLITE_OK)
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

inline void bind_int(sqlite3 *db, sqlite3_stmt *stmt, int idx, int value, const std::string &context)
{
    if (sqlite3_bind_int(stmt, idx, value) != SQLITE_OK)
    {
        throw_sqlite_error(db, context);
    }
}

inline void bind_text(sqlite3 *db, sqlite3_stmt *stmt, int idx, const std::string &value, const std::string &context)
{
    if (sqlite3_bind_text(stmt, idx, value.c_str(), -1, SQLITE_TRANSIENT) != SQLITE_OK)
    {
        throw_sqlite_error(db, context);
    }
}

inline bool step_is_row_or_done(sqlite3 *db, sqlite3_stmt *stmt, const std::string &context)
{
    const int result = call_with_sqlite_lock_retry(
        [&]()
        {
            return sqlite3_step(stmt);
        });

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

inline void step_expect_done(sqlite3 *db, sqlite3_stmt *stmt, const std::string &context)
{
    const int result = call_with_sqlite_lock_retry(
        [&]()
        {
            return sqlite3_step(stmt);
        });

    if (result != SQLITE_DONE)
    {
        throw_sqlite_error(db, context);
    }
}
