SentenceIterator::SentenceIterator(
    int8_t file_idx, int node_set_idx) : file_idx(file_idx),
                                         node_set_idx(node_set_idx)
{
    if (file_idx < 0 || file_idx >= static_cast<int8_t>(LCMC_FILENAMES.size()))
    {
        return;
    }

    Logger logger("SentenceIterator(" + LCMC_FILENAMES.at(file_idx) + ")");

    logger.Debug("Initializing `SentenceIterator`");

    parser = std::make_shared<xmlpp::DomParser>();
    parser->parse_file(LCMC_BASEDIR + "/" + LCMC_FILENAMES.at(file_idx));
    if (!parser->get_document() || !parser->get_document()->get_root_node())
    {
        throw ParserException("[SentenceIterator] Failed to parse sentence XML");
        return;
    }

    curr_node_set = parser->get_document()->get_root_node()->find("//s");

    logger.Debug("`SentenceIterator` initialized");
}

int8_t SentenceIterator::get_file_idx() const
{
    return file_idx;
}

int SentenceIterator::get_curr_node_set_idx() const
{
    return node_set_idx;
}

SentenceIterator SentenceIterator::begin() const
{
    SentenceIterator iterator(file_idx, 0);
    if (!iterator.curr_node_set.empty())
    {
        return iterator;
    }

    return ++iterator;
}

SentenceIterator SentenceIterator::end() const
{
    return SentenceIterator(file_idx, curr_node_set.size());
}

SentenceIterator &SentenceIterator::operator++()
{
    node_set_idx++;
    return *this;
}

Sentence SentenceIterator::operator*() const
{
    Logger logger("SentenceIterator(" + LCMC_FILENAMES.at(file_idx) + ")::operator*");

    if (node_set_idx % 50 == 0 || node_set_idx == curr_node_set.size() - 1)
    {
        logger.Info("Instantiating `Sequence` for index {}/{}",
                    node_set_idx, curr_node_set.size() - 1);
    }

    xmlpp::Node *sentence_node = curr_node_set.at(node_set_idx);
    WordIterator iterator(sentence_node->find(".//w"), 0);
    std::vector<Word> word_seq;
    std::string sentence_value;

    for (Word word_node : iterator)
    {
        if (!sentence_value.empty())
        {
            sentence_value += " ";
        }

        sentence_value += word_node.get_value();
        word_seq.push_back(word_node);
    }

    if (!word_seq.empty())
    {
        return Sentence(sentence_value, word_seq);
    }

    logger.Debug("Skipping sentence without words at node index {}/{}",
                 node_set_idx, curr_node_set.size() - 1);
    return Sentence("", {});
}

bool SentenceIterator::operator!=(const SentenceIterator &rhs) const
{
    return get_file_idx() != rhs.get_file_idx() ||
           get_curr_node_set_idx() != rhs.get_curr_node_set_idx();
}
