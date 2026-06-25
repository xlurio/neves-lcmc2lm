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
