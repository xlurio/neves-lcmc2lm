void Word::Save(sqlite3 *db)
{
    Logger logger("Word('" + value + "')::Save");

    logger.Debug("Saving");

    if (!ShouldPersist())
    {
        return;
    }

    run_in_immediate_transaction(
        db,
        "[Word::Save] immediate upsert",
        [this, db]()
        {
            int persisted_hash = hash;
            constexpr int max_probe_attempts = 2048;

            for (int attempt = 0; attempt < max_probe_attempts; ++attempt)
            {
                const int changes = execute_non_query_with_changes(
                    db,
                    UPSERT_WORD_QUERY,
                    [this, persisted_hash, db](sqlite3_stmt *stmt)
                    {
                        bind_int(
                            db,
                            stmt,
                            1,
                            persisted_hash,
                            "[Word::Save] Failed to bind word mutation ID");
                        bind_text(
                            db,
                            stmt,
                            2,
                            value,
                            "[Word::Save] Failed to bind word value");
                        bind_text(
                            db,
                            stmt,
                            3,
                            get_verbose_pos_tag(),
                            "[Word::Save] Failed to bind word POS tag");
                    },
                    "[Word::Save] Failed to persist word");

                if (changes > 0)
                {
                    hash = persisted_hash;
                    return;
                }

                const std::pair<bool, std::string> existing_word = optional_text_value(
                    db,
                    GET_WORD_BY_ID_QUERY,
                    [persisted_hash, db](sqlite3_stmt *stmt)
                    {
                        bind_int(db, stmt, 1, persisted_hash, "[Word::Save] Failed to bind word ID");
                    },
                    0,
                    "[Word::Save] Failed to select word");

                if (!existing_word.first)
                {
                    throw DatabaseException("[Word::Save] Word upsert did not affect rows and no existing row was found");
                }

                if (existing_word.second == value)
                {
                    hash = persisted_hash;
                    return;
                }

                persisted_hash = next_hash_candidate(persisted_hash);
            }

            throw DatabaseException("[Word::Save] Exhausted hash collision probe attempts for value '" + value + "'");
        });

    for (mcc2lm::Logogram &logogram : logograms)
    {
        if (!logogram.ShouldPersist())
        {
            continue;
        }

        logogram.Save(db);

        ensure_mapping_row(
            db,
            INSERT_LOGOGRAM_WORD_MAP_QUERY,
            [this, &logogram, db](sqlite3_stmt *stmt)
            {
                bind_int(db, stmt, 1, hash, "[Word::Save] Failed to bind logogram-word map word ID");
                bind_text(db, stmt, 2, logogram.get_id(), "[Word::Save] Failed to bind logogram-word map logogram ID");
            },
            "[Word::Save] logogram-word map mutation");
    }

    logger.Debug("Saved");
}
