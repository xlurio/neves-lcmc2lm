#include <cstdint>
#include <exception>
#include <mutex>
#include <string>
#include <thread>
#include <vector>
#include <sqlite3.h>
#include <mcc2lm/constants.hpp>
#include <mcc2lm/exceptions.hpp>
#include <mcc2lm/logger.hpp>
#include <mcc2lm/sentence.hpp>
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

    void process_file_worker(std::size_t file_idx, std::exception_ptr &first_worker_exception, std::mutex &exception_mutex)
    {
        try
        {
            mcc2lm::Logger logger("process_file_worker(" + mcc2lm::LCMC_FILENAMES.at(file_idx) + ")");

            ScopedSqliteConnection thread_connection("mcc2lm.db", "[worker] Failed to open database");
            configure_sqlite_connection(thread_connection.get(), "[worker]");

            logger.Debug("Processing LCMC file");

            mcc2lm::SentenceIterator iterator(static_cast<int8_t>(file_idx), 0);
            std::size_t processed_sentence_count = 0;
            std::size_t skipped_sentence_count = 0;

            for (mcc2lm::Sentence sentence : iterator)
            {
                if (!sentence.ShouldPersist())
                {
                    ++skipped_sentence_count;
                    continue;
                }

                sentence.Save(thread_connection.get());
                ++processed_sentence_count;
            }

            logger.Info(
                "LCMC file processed (saved: {}, skipped: {})",
                processed_sentence_count,
                skipped_sentence_count);
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
}

void process_all_files()
{
    std::mutex exception_mutex;
    std::exception_ptr first_worker_exception;
    std::vector<std::thread> workers;
    workers.reserve(mcc2lm::LCMC_FILENAMES.size());

    for (std::size_t file_idx = 0; file_idx < mcc2lm::LCMC_FILENAMES.size(); ++file_idx)
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
