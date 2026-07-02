#include <sqlite3.h>
#include <string>
#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/sql.hpp>

namespace
{
    constexpr int SQLITE_BUSY_TIMEOUT_MS = 30000;

    class ScopedSqliteConnection
    {
        sqlite3 *db = nullptr;

    public:
        ScopedSqliteConnection(const char *path, const std::string &open_error_context)
        {
            if (sqlite3_open(path, &db) != SQLITE_OK)
            {
                throw mcc2lm::DatabaseException(open_error_context);
            }
        }

        ~ScopedSqliteConnection()
        {
            if (db != nullptr)
            {
                sqlite3_close(db);
            }
        }

        ScopedSqliteConnection(const ScopedSqliteConnection &) = delete;
        ScopedSqliteConnection &operator=(const ScopedSqliteConnection &) = delete;

        sqlite3 *get() const
        {
            return db;
        }
    };

    void configure_sqlite_connection(sqlite3 *db, const std::string &context)
    {
        mcc2lm::execute_sqlite_exec(db, "PRAGMA foreign_keys = ON;", context + " Failed to enable foreign keys");
        mcc2lm::execute_sqlite_exec(db, "PRAGMA journal_mode = WAL;", context + " Failed to enable WAL mode");
        mcc2lm::execute_sqlite_exec(db, "PRAGMA synchronous = NORMAL;", context + " Failed to configure synchronous mode");

        if (sqlite3_busy_timeout(db, SQLITE_BUSY_TIMEOUT_MS) != SQLITE_OK)
        {
            throw mcc2lm::DatabaseException(context + " Failed to configure busy timeout");
        }
    }
}

void initialize_database_schema()
{
    const std::string create_radical_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_RADICAL("
        "ID VARCHAR(4) PRIMARY KEY,"
        "PINYIN TEXT,"
        "MEANING TEXT);";
    const std::string create_logogram_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_LOGOGRAM("
        "ID VARCHAR(4) PRIMARY KEY,"
        "OCCURRENCIES INTEGER,"
        "PINYIN TEXT,"
        "MEANING TEXT);";
    const std::string create_radical_logogram_map_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_RADICAL_LOGOGRAM_MAP("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "LOGOGRAM_ID VARCHAR(4),"
        "RADICAL_ID VARCHAR(4),"
        "UNIQUE(LOGOGRAM_ID, RADICAL_ID),"
        "FOREIGN KEY(LOGOGRAM_ID) REFERENCES MCC2LM_LOGOGRAM(ID),"
        "FOREIGN KEY(RADICAL_ID) REFERENCES MCC2LM_RADICAL(ID));";
    const std::string create_word_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_WORD("
        "ID INTEGER PRIMARY KEY,"
        "VALUE TEXT,"
        "POS_TAG VARCHAR(255),"
        "OCCURRENCIES INTEGER);";
    const std::string create_logogram_word_map_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_LOGOGRAM_WORD_MAP("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "WORD_ID INTEGER,"
        "LOGOGRAM_ID VARCHAR(4),"
        "UNIQUE(WORD_ID, LOGOGRAM_ID),"
        "FOREIGN KEY(WORD_ID) REFERENCES MCC2LM_WORD(ID),"
        "FOREIGN KEY(LOGOGRAM_ID) REFERENCES MCC2LM_LOGOGRAM(ID));";
    const std::string create_ngram_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_NGRAM("
        "ID INTEGER PRIMARY KEY," // hash
        "VALUE TEXT,"             // Raw n-gram
        "N_WORDS INTEGER,"        // 3 for 3-grams and 4 for 4-grams
        "OCCURRENCIES INTEGER);";
    const std::string create_word_ngram_map_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_WORD_NGRAM_MAP("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "NGRAM_ID INTEGER,"
        "WORD_ID INTEGER,"
        "UNIQUE(NGRAM_ID, WORD_ID),"
        "FOREIGN KEY(NGRAM_ID) REFERENCES MCC2LM_NGRAM(ID),"
        "FOREIGN KEY(WORD_ID) REFERENCES MCC2LM_WORD(ID));";
    const std::string create_sentence_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_SENTENCE("
        "ID INTEGER PRIMARY KEY,"
        "VALUE TEXT);";
    const std::string create_word_sentence_map_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_WORD_SENTENCE_MAP("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "SENTENCE_ID INTEGER,"
        "WORD_ID INTEGER,"
        "UNIQUE(SENTENCE_ID, WORD_ID),"
        "FOREIGN KEY(SENTENCE_ID) REFERENCES MCC2LM_SENTENCE(ID),"
        "FOREIGN KEY(WORD_ID) REFERENCES MCC2LM_WORD(ID));";
    const std::string create_ngram_sentence_map_table_query =
        "CREATE TABLE IF NOT EXISTS MCC2LM_NGRAM_SENTENCE_MAP("
        "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
        "NGRAM_ID INTEGER,"
        "SENTENCE_ID INTEGER,"
        "UNIQUE(NGRAM_ID, SENTENCE_ID),"
        "FOREIGN KEY(NGRAM_ID) REFERENCES MCC2LM_NGRAM(ID),"
        "FOREIGN KEY(SENTENCE_ID) REFERENCES MCC2LM_SENTENCE(ID));";

    ScopedSqliteConnection connection("mcc2lm.db", "[main] Failed to open database");
    configure_sqlite_connection(connection.get(), "[main]");

    mcc2lm::execute_sqlite_exec(
        connection.get(),
        create_radical_table_query                    //
            + create_logogram_table_query             //
            + create_radical_logogram_map_table_query //
            + create_word_table_query                 //
            + create_logogram_word_map_table_query    //
            + create_ngram_table_query                //
            + create_word_ngram_map_table_query       //
            + create_sentence_table_query             //
            + create_word_sentence_map_table_query    //
            + create_word_ngram_map_table_query       //
            + create_ngram_sentence_map_table_query,
        "[main] Failed to create database schema");

    const auto has_column = [db = connection.get()](const std::string &table_name, const std::string &column_name)
    {
        return mcc2lm::row_exists(
            db,
            "SELECT 1 FROM pragma_table_info('" + table_name + "') WHERE name = ? LIMIT 1;",
            [&column_name, db](sqlite3_stmt *stmt)
            {
                mcc2lm::bind_text(db, stmt, 1, column_name, "[main] Failed to bind schema column name");
            },
            "[main] Failed to inspect schema columns");
    };

    const auto ensure_column = [db = connection.get(), &has_column](const std::string &table_name, const std::string &column_definition)
    {
        const std::string column_name = column_definition.substr(0, column_definition.find(' '));
        if (column_name.empty() || has_column(table_name, column_name))
        {
            return;
        }

        mcc2lm::execute_sqlite_exec(
            db,
            "ALTER TABLE " + table_name + " ADD COLUMN " + column_definition + ";",
            "[main] Failed to migrate schema");
    };

    ensure_column("MCC2LM_RADICAL", "PINYIN TEXT");
    ensure_column("MCC2LM_RADICAL", "MEANING TEXT");
    ensure_column("MCC2LM_LOGOGRAM", "PINYIN TEXT");
    ensure_column("MCC2LM_LOGOGRAM", "MEANING TEXT");
}
