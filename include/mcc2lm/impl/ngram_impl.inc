inline std::vector<mcc2lm::Ngram> extract_ngrams_from_sentence(const std::vector<mcc2lm::Word> &words)
{
    std::vector<mcc2lm::Ngram> ngrams;

    // Extract 3-grams and 4-grams from ordered word sequence
    if (words.size() >= 3)
    {
        // Extract 3-grams
        for (size_t i = 0; i <= words.size() - 3; ++i)
        {
            std::vector<mcc2lm::Word> window(words.begin() + i, words.begin() + i + 3);
            ngrams.emplace_back(window, 3);
        }
    }

    if (words.size() >= 4)
    {
        // Extract 4-grams
        for (size_t i = 0; i <= words.size() - 4; ++i)
        {
            std::vector<mcc2lm::Word> window(words.begin() + i, words.begin() + i + 4);
            ngrams.emplace_back(window, 4);
        }
    }

    return ngrams;
}
void Ngram::Save(sqlite3 *db)
{
    Logger logger("Ngram('" + value + "')::Save");

    logger.Debug("Saving");

    if (!ShouldPersist())
    {
        logger.Debug("N-gram skipped (n_words=" + std::to_string(n_words) + " not in {3,4})");
        return;
    }

    run_in_immediate_transaction(
        db,
        "[Ngram::Save] immediate upsert",
        [this, db]()
        {
            int persisted_hash = hash;
            constexpr int max_probe_attempts = 2048;

            for (int attempt = 0; attempt < max_probe_attempts; ++attempt)
            {
                const int changes = execute_non_query_with_changes(
                    db,
                    UPSERT_NGRAM_QUERY,
                    [this, persisted_hash, db](sqlite3_stmt *stmt)
                    {
                        bind_int(
                            db,
                            stmt,
                            1,
                            persisted_hash,
                            "[Ngram::Save] Failed to bind n-gram ID");
                        bind_text(
                            db,
                            stmt,
                            2,
                            value,
                            "[Ngram::Save] Failed to bind n-gram value");
                        bind_int(
                            db,
                            stmt,
                            3,
                            n_words,
                            "[Ngram::Save] Failed to bind n-gram word count");
                    },
                    "[Ngram::Save] Failed to persist n-gram");

                if (changes > 0)
                {
                    hash = persisted_hash;
                    return;
                }

                const std::pair<bool, std::string> existing_ngram = optional_text_value(
                    db,
                    GET_NGRAM_VALUE_QUERY,
                    [persisted_hash, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, persisted_hash, "[Ngram::Save] Failed to bind n-gram ID for select");
                    },
                    0,
                    "[Ngram::Save] Failed to select n-gram");

                if (!existing_ngram.first)
                {
                    throw DatabaseException("[Ngram::Save] N-gram upsert did not affect rows and no existing row was found");
                }

                if (existing_ngram.second == value)
                {
                    hash = persisted_hash;
                    return;
                }

                persisted_hash = next_hash_candidate(persisted_hash);
            }

            throw DatabaseException("[Ngram::Save] Exhausted hash collision probe attempts for value '" + value + "'");
        });

    logger.Debug("Saved");
}
int Ngram::hash_from_value() const
{
    const std::size_t raw_hash = std::hash<std::string>{}(value);
    return static_cast<int>(raw_hash % INT_MAX);
}

int Ngram::next_hash_candidate(int current_hash) const
{
    return (current_hash + 1) % INT_MAX;
}

Ngram::Ngram(const std::vector<Word> &words, int n_words_count) : n_words(n_words_count)
{
    Logger logger("Ngram");

    if (static_cast<int>(words.size()) != n_words_count)
    {
        throw DatabaseException("[Ngram] N-gram word count mismatch: expected " + 
                               std::to_string(n_words_count) + " but got " + 
                               std::to_string(words.size()));
    }

    component_word_hashes.reserve(words.size());
    value = "";

    for (size_t i = 0; i < words.size(); ++i)
    {
        component_word_hashes.push_back(words[i].get_hash());
        if (i > 0)
        {
            value += " ";
        }
        value += words[i].get_value();
    }

    hash = hash_from_value();

    logger.Debug("[Ngram] Initialized n-gram with VALUE='" + value + "', N_WORDS=" + 
                std::to_string(n_words) + ", HASH=" + std::to_string(hash));
}

int Ngram::get_hash() const
{
    return hash;
}

std::string Ngram::get_value() const
{
    return value;
}

int Ngram::get_n_words() const
{
    return n_words;
}

bool Ngram::ShouldPersist() const
{
    return n_words == 3 || n_words == 4;
}
