
    logger.Debug("Saved");
}
WordPosTag Word::PosTagFromRaw(std::string raw_pos_tag)
{
    if (raw_pos_tag == "a" || raw_pos_tag == "ad" || raw_pos_tag == "ag" ||
        raw_pos_tag == "an" || raw_pos_tag == "b" || raw_pos_tag == "bg")
    {
        return WordPosTag::ADJECTIVE;
    }
    else if (raw_pos_tag == "d" || raw_pos_tag == "dg")
    {
        return WordPosTag::ADVERB;
    }
    else if (raw_pos_tag == "c" || raw_pos_tag == "cg")
    {
        return WordPosTag::CONJUCTION;
    }
    else if (raw_pos_tag == "e")
    {
        return WordPosTag::INTERJECTION;
    }
    else if (raw_pos_tag == "y" || raw_pos_tag == "yg" || raw_pos_tag == "z" ||
             raw_pos_tag == "zg")
    {
        return WordPosTag::MODAL_PARTICLE;
    }
    else if (raw_pos_tag == "m" || raw_pos_tag == "mg")
    {
        return WordPosTag::NUMERAL;
    }
    else if (raw_pos_tag == "o")
    {
        return WordPosTag::ONOMATOPOEIA;
    }
    else if (raw_pos_tag == "p" || raw_pos_tag == "pg")
    {
        return WordPosTag::PREPOSITION;
    }
    else if (raw_pos_tag == "r" || raw_pos_tag == "rg")
    {
        return WordPosTag::PRONOUN;
    }
    else if (raw_pos_tag == "u" || raw_pos_tag == "v" || raw_pos_tag == "vd" ||
             raw_pos_tag == "vg" || raw_pos_tag == "vn")
    {
        return WordPosTag::VERB;
    }
    else if (raw_pos_tag == "x")
    {
        return WordPosTag::UNCLASSIFIED;
    }
    else if (raw_pos_tag == "ew" || raw_pos_tag == "h" || raw_pos_tag == "k" ||
             raw_pos_tag == "w")
    {
        return WordPosTag::IGNORE;
    }

    return WordPosTag::NOUN;
}

int Word::hash_from_value() const
{
    const std::size_t raw_hash = std::hash<std::string>{}(value);
    return static_cast<int>(raw_hash % INT_MAX);
}

int Word::next_hash_candidate(int current_hash) const
{
    if (current_hash >= INT_MAX - 1)
    {
        return 1;
    }

    return current_hash + 1;
}

Word::Word(std::vector<mcc2lm::Logogram> logograms,
           std::string value,
           std::string raw_pos_tag) : logograms(logograms),
                                      value(value)
{
    Logger logger("Word(" + value + ")");

    logger.Debug("Initializing `Word`");

    pos_tag = PosTagFromRaw(raw_pos_tag);
    hash = hash_from_value();

    logger.Debug("`Word` initialized");
}

int Word::get_hash() const
{
    return hash;
}

std::string Word::get_verbose_pos_tag() const
{
    switch (pos_tag)
    {
    case WordPosTag::ADJECTIVE:
        return "ADJECTIVE";
    case WordPosTag::ADVERB:
        return "ADVERB";
    case WordPosTag::CONJUCTION:
        return "CONJUCTION";
    case WordPosTag::INTERJECTION:
        return "INTERJECTION";
    case WordPosTag::MODAL_PARTICLE:
        return "MODAL_PARTICLE";
    case WordPosTag::NOUN:
        return "NOUN";
    case WordPosTag::NUMERAL:
        return "NUMERAL";
    case WordPosTag::ONOMATOPOEIA:
        return "ONOMATOPOEIA";
    case WordPosTag::PREPOSITION:
        return "PREPOSITION";
    case WordPosTag::PRONOUN:
        return "PRONOUN";
    case WordPosTag::VERB:
        return "VERB";
    case WordPosTag::UNCLASSIFIED:
        return "UNCLASSIFIED";
    }

    return "IGNORE";
}

bool Word::ShouldPersist() const
{
    if (value.empty() || pos_tag == WordPosTag::IGNORE)
    {
        return false;
    }

    for (const mcc2lm::Logogram &logogram : logograms)
    {
        if (logogram.ShouldPersist())
        {
            return true;
        }
    }

    return false;
}

const std::string &Word::get_value() const
{
    return value;
}

Word Word::MakeFromRawStrAndPosTag(std::string raw_str, std::string raw_pos_tag)
{
    std::vector<mcc2lm::Logogram> logogram_seq;
    mcc2lm::LogogramIterator iterator(raw_str, 0);

    for (mcc2lm::Logogram logograph : iterator)
    {
        logogram_seq.push_back(logograph);
    }

    return Word(
        logogram_seq,
        raw_str,
        raw_pos_tag);
}
