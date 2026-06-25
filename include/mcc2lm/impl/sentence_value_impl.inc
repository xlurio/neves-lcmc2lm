int Sentence::hash_from_unordered_words() const
{
    std::vector<std::string> sorted_values;
    sorted_values.reserve(words.size());

    for (const Word &word : words)
    {
        sorted_values.push_back(word.get_value());
    }

    std::sort(sorted_values.begin(), sorted_values.end());

    std::string canonical;
    for (const std::string &word_value : sorted_values)
    {
        canonical.push_back('\x1f');
        canonical += word_value;
    }

    const std::size_t raw_hash = std::hash<std::string>{}(canonical);
    return static_cast<int>(raw_hash % INT_MAX);
}

Sentence::Sentence(std::string value, std::vector<Word> words) : value(value),
                                                                 words(words)
{
    Logger logger("Sentence('" + value + "')");

    hash = hash_from_unordered_words();

    logger.Debug("`Sentence` initialized");
}

int Sentence::get_hash() const
{
    return hash;
}

bool Sentence::ShouldPersist() const
{
    return !words.empty();
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

    logger.Debug("Saved");
}
