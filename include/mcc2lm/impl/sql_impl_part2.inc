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
inline bool row_exists(
    sqlite3 *db,
    const std::string &query,
    const std::function<void(sqlite3_stmt *)> &bind_params,
    const std::string &context)
{
    SqliteStatement stmt(db, query, context + " [prepare]");
    bind_params(stmt.get());
    return step_is_row_or_done(db, stmt.get(), context + " [step]");
}

inline std::pair<bool, std::string> optional_text_value(
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

inline void execute_non_query(
    sqlite3 *db,
    const std::string &query,
    const std::function<void(sqlite3_stmt *)> &bind_params,
    const std::string &context)
{
    SqliteStatement stmt(db, query, context + " [prepare]");
    bind_params(stmt.get());
    step_expect_done(db, stmt.get(), context + " [step]");
}

inline int execute_non_query_with_changes(
    sqlite3 *db,
    const std::string &query,
    const std::function<void(sqlite3_stmt *)> &bind_params,
    const std::string &context)
{
    SqliteStatement stmt(db, query, context + " [prepare]");
    bind_params(stmt.get());
    step_expect_done(db, stmt.get(), context + " [step]");
    return sqlite3_changes(db);
}

inline void ensure_mapping_row(
    sqlite3 *db,
    const std::string &insert_query,
    const std::function<void(sqlite3_stmt *)> &bind_keys,
    const std::string &context)
{
    execute_non_query(db, insert_query, bind_keys, context + " [insert]");
}
template <typename Func>
void run_in_immediate_transaction(sqlite3 *db, const std::string &context, Func &&body)
{
    if (sqlite3_get_autocommit(db) == 0)
    {
        body();
        return;
    }

    execute_sqlite_exec(db, "BEGIN IMMEDIATE;", context + " [begin] Failed to start immediate transaction");

    try
    {
        body();
    }
    catch (...)
    {
        if (sqlite3_get_autocommit(db) == 0)
        {
            execute_sqlite_exec(db, "ROLLBACK;", context + " [rollback] Failed to rollback immediate transaction");
        }

        throw;
    }

    try
    {
        execute_sqlite_exec(db, "COMMIT;", context + " [commit] Failed to commit immediate transaction");
    }
    catch (...)
    {
        if (sqlite3_get_autocommit(db) == 0)
        {
            (void)sqlite3_exec(db, "ROLLBACK;", nullptr, nullptr, nullptr);
        }

        throw;
    }
}
