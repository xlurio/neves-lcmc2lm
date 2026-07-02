}

int Sentence::get_hash() const
{
    return hash;
}

bool Sentence::ShouldPersist() const
{
    for (const Word &word : words)
    {
        if (word.ShouldPersist())
        {
            return true;
        }
    }
    return false;
}

void Sentence::Save(sqlite3 *db)
{
    Logger logger("Sentence('" + value + "')::Save");

    logger.Debug("Saving");

    if (!ShouldPersist())
    {
        return;
    }

    execute_non_query(
        db,
        INSERT_SENTENCE_QUERY,
        [this, db](sqlite3_stmt *stmt)
        {
            bind_int(db, stmt, 1, hash, "[Sentence::Save] Failed to bind sentence insert ID");
            bind_text(db, stmt, 2, value, "[Sentence::Save] Failed to bind sentence value");
        },
        "[Sentence::Save] Failed to insert sentence");

    for (Word &word : words)
    {
        word.Save(db);

        if (!word.ShouldPersist())
        {
            continue;
        }

        ensure_mapping_row(
            db,
            INSERT_WORD_SENTENCE_MAP_QUERY,
            [this, &word, db](sqlite3_stmt *stmt)
            {
                bind_int(db, stmt, 1, hash, "[Sentence::Save] Failed to bind sentence-word map sentence ID");
                bind_int(db, stmt, 2, word.get_hash(), "[Sentence::Save] Failed to bind sentence-word map word ID");
            },
            "[Sentence::Save] sentence-word map mutation");
    }

    // Extract and persist n-grams (3-grams and 4-grams)
    try
    {
        if (words.size() >= 3)
        {
            // Extract 3-grams
            for (size_t i = 0; i <= words.size() - 3; ++i)
            {
                std::vector<Word> window(words.begin() + i, words.begin() + i + 3);
                mcc2lm::Ngram ngram_3(window, 3);
                ngram_3.Save(db);

                // Map each component word to the 3-gram
                for (const Word &word : window)
                {
                    if (!word.ShouldPersist())
                    {
                        continue;
                    }
                    ensure_mapping_row(
                        db,
                        mcc2lm::Ngram::WORD_NGRAM_MAP_INSERT_QUERY,
                        [&ngram_3, &word, db](sqlite3_stmt *stmt)
                        {
                            bind_int(db, stmt, 1, ngram_3.get_hash(), "[Sentence::Save] Failed to bind 3-gram word map n-gram ID");
                            bind_int(db, stmt, 2, word.get_hash(), "[Sentence::Save] Failed to bind 3-gram word map word ID");
                        },
                        "[Sentence::Save] 3-gram word map mutation");
                }
            }
        }

        if (words.size() >= 4)
        {
            // Extract 4-grams
            for (size_t i = 0; i <= words.size() - 4; ++i)
            {
                std::vector<Word> window(words.begin() + i, words.begin() + i + 4);
                mcc2lm::Ngram ngram_4(window, 4);
                ngram_4.Save(db);

                // Map each component word to the 4-gram
                for (const Word &word : window)
                {
                    if (!word.ShouldPersist())
                    {
                        continue;
                    }
                    ensure_mapping_row(
                        db,
                        mcc2lm::Ngram::WORD_NGRAM_MAP_INSERT_QUERY,
                        [&ngram_4, &word, db](sqlite3_stmt *stmt)
                        {
                            bind_int(db, stmt, 1, ngram_4.get_hash(), "[Sentence::Save] Failed to bind 4-gram word map n-gram ID");
                            bind_int(db, stmt, 2, word.get_hash(), "[Sentence::Save] Failed to bind 4-gram word map word ID");
                        },
                        "[Sentence::Save] 4-gram word map mutation");
                }
            }
        }
    }
    catch (const mcc2lm::DatabaseException &e)
    {
        logger.Error("N-gram extraction and persistence failed: " + std::string(e.what()) + " - continuing");
    }

    logger.Debug("Saved");
}
