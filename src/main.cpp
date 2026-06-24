#include <array>
#include <cstdint>
#include <cstdlib>
#include <exception>
#include <functional>
#include <mutex>
#include <string>
#include <thread>
#include <vector>

#include <sqlite3.h>

#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/sentence.hpp>

// CREATE TABLE
const std::string CREATE_LOGOGRAM_TABLE_QUERY =
    "CREATE TABLE IF NOT EXISTS MCC2LM_LOGOGRAM("
    "ID VARCHAR(255) PRIMARY KEY,"
    "OCCURRENCIES INTEGER);";
const std::string CREATE_WORD_TABLE_QUERY = "CREATE TABLE IF NOT EXISTS MCC2LM_WORD("
                                            "ID INTEGER PRIMARY KEY,"
                                            "VALUE TEXT,"
                                            "OCCURRENCIES INTEGER);";
const std::string CREATE_LOGOGRAM_WORD_MAP_TABLE_QUERY =
    "CREATE TABLE IF NOT EXISTS MCC2LM_LOGOGRAM_WORD_MAP("
    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
    "WORD_ID INTEGER,"
    "LOGOGRAM_ID VARCHAR(255),"
    "UNIQUE(WORD_ID, LOGOGRAM_ID),"
    "FOREIGN KEY(WORD_ID) REFERENCES MCC2LM_WORD(ID),"
    "FOREIGN KEY(LOGOGRAM_ID) REFERENCES MCC2LM_LOGOGRAM(ID));";
const std::string CREATE_SENTENCE_TABLE_QUERY =
    "CREATE TABLE IF NOT EXISTS MCC2LM_SENTENCE("
    "ID INTEGER PRIMARY KEY,"
    "VALUE TEXT);";
const std::string CREATE_WORD_SENTENCE_MAP_TABLE_QUERY =
    "CREATE TABLE IF NOT EXISTS MCC2LM_WORD_SENTENCE_MAP("
    "ID INTEGER PRIMARY KEY AUTOINCREMENT,"
    "SENTENCE_ID INTEGER,"
    "WORD_ID INTEGER,"
    "UNIQUE(SENTENCE_ID, WORD_ID),"
    "FOREIGN KEY(SENTENCE_ID) REFERENCES MCC2LM_SENTENCE(ID),"
    "FOREIGN KEY(WORD_ID) REFERENCES MCC2LM_WORD(ID));";
// ---

const std::string LCMC_BASEDIR = "assets//data/character";

const std::array<std::string, 15> LCMC_FILENAMES = {
    "LCMC_A.XML",
    "LCMC_B.XML",
    "LCMC_C.XML",
    "LCMC_D.XML",
    "LCMC_E.XML",
    "LCMC_F.XML",
    "LCMC_G.XML",
    "LCMC_H.XML",
    "LCMC_J.XML",
    "LCMC_K.XML",
    "LCMC_L.XML",
    "LCMC_M.XML",
    "LCMC_N.XML",
    "LCMC_P.XML",
    "LCMC_R.XML"};

namespace
{
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

    void initialize_database_schema()
    {
        ScopedSqliteConnection connection("mcc2lm.db", "[main] Failed to open database");

        if (sqlite3_exec(connection.get(), "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            throw mcc2lm::DatabaseException("[main] Failed to enable foreign keys");
        }

        if (sqlite3_exec(
                connection.get(),
                (CREATE_LOGOGRAM_TABLE_QUERY            //
                 + CREATE_WORD_TABLE_QUERY              //
                 + CREATE_LOGOGRAM_WORD_MAP_TABLE_QUERY //
                 + CREATE_SENTENCE_TABLE_QUERY          //
                 + CREATE_WORD_SENTENCE_MAP_TABLE_QUERY)
                    .c_str(),
                nullptr, nullptr, nullptr) != SQLITE_OK)
        {
            throw mcc2lm::DatabaseException("[main] Failed to create logogram table");
        }
    }

    void process_file_worker(std::size_t file_idx, std::exception_ptr &first_worker_exception, std::mutex &exception_mutex)
    {
        try
        {
            ScopedSqliteConnection thread_connection("mcc2lm.db", "[worker] Failed to open database");

            if (sqlite3_exec(thread_connection.get(), "PRAGMA foreign_keys = ON;", nullptr, nullptr, nullptr) != SQLITE_OK)
            {
                throw mcc2lm::DatabaseException("[worker] Failed to enable foreign keys");
            }

            if (sqlite3_busy_timeout(thread_connection.get(), 5000) != SQLITE_OK)
            {
                throw mcc2lm::DatabaseException("[worker] Failed to configure busy timeout");
            }

            mcc2lm::SentenceIterator iterator(static_cast<int8_t>(file_idx), 0);
            for (mcc2lm::Sentence sentence : iterator)
            {
                sentence.save(thread_connection.get());
            }
        }
        catch (...)
        {
            std::lock_guard<std::mutex> lock(exception_mutex);
            if (first_worker_exception == nullptr)
            {
                first_worker_exception = std::current_exception();
            }
        }
    }

    void process_all_files()
    {
        std::mutex exception_mutex;
        std::exception_ptr first_worker_exception;
        std::vector<std::thread> workers;
        workers.reserve(LCMC_FILENAMES.size());

        for (std::size_t file_idx = 0; file_idx < LCMC_FILENAMES.size(); ++file_idx)
        {
            workers.emplace_back(process_file_worker, file_idx, std::ref(first_worker_exception), std::ref(exception_mutex));
        }

        for (std::thread &worker : workers)
        {
            worker.join();
        }

        if (first_worker_exception != nullptr)
        {
            std::rethrow_exception(first_worker_exception);
        }
    }
}

int main()
{
    initialize_database_schema();
    process_all_files();

    return EXIT_SUCCESS;
}
