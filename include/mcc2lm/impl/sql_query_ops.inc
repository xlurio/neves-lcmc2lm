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
