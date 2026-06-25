Logogram::Logogram(std::string value) : value(value)
{
    Logger logger("Logogram(" + value + ")");

    const CharacterMetadata metadata = get_unihan_metadata(value);
    pinyin = metadata.pinyin;
    meaning = metadata.meaning;

    logger.Debug("`Logogram` initialized");
}

const std::string &Logogram::get_id() const
{
    return value;
}

bool Logogram::ShouldPersist() const
{
    return !value.empty() && !pinyin.empty() && !meaning.empty();
}

void Logogram::Save(sqlite3 *db)
{
    Logger logger("Logogram('" + value + "')::Save");

    logger.Debug("Saving");

    if (!ShouldPersist())
    {
        return;
    }

    execute_non_query(
        db,
        UPSERT_LOGOGRAM_QUERY,
        [this, db](sqlite3_stmt *stmt)
        {
            bind_text(
                db,
                stmt,
                1,
                value,
                "[Logogram::Save] Failed to bind value mutation ID");
            bind_text(
                db,
                stmt,
                2,
                pinyin,
                "[Logogram::Save] Failed to bind value pinyin");
            bind_text(
                db,
                stmt,
                3,
                meaning,
                "[Logogram::Save] Failed to bind value meaning");
        },
        "[Logogram::Save] Failed to persist value");

    RadicalIterator radicals(value, 0);

    for (Radical radical : radicals)
    {
        radical.Save(db);

        ensure_mapping_row(
            db,
            INSERT_RADICAL_LOGOGRAM_MAP_QUERY,
            [this, &radical, db](sqlite3_stmt *stmt)
            {
                bind_text(
                    db,
                    stmt,
                    1,
                    value,
                    "[Word::Save] Failed to bind radical-logogram map logogram ID");
                bind_text(
                    db,
                    stmt,
                    2,
                    radical.get_value(),
                    "[Word::Save] Failed to bind radical-logogram map radical ID");
            },
            "[Word::Save] radical-logogram map mutation");
    }

    logger.Debug("Saved");
}
