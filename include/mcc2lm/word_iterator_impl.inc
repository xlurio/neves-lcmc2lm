WordIterator::WordIterator(
    xmlpp::Node::NodeSet word_node_set,
    int curr_idx) : word_node_set(word_node_set),
                    curr_idx(curr_idx) {}

int WordIterator::get_curr_idx() const
{
    return curr_idx;
}

WordIterator WordIterator::begin() const
{
    return WordIterator(word_node_set, 0);
}

WordIterator WordIterator::end() const
{
    return WordIterator(word_node_set, word_node_set.size());
}

WordIterator &WordIterator::operator++()
{
    curr_idx++;
    return *this;
}

Word WordIterator::operator*() const
{
    xmlpp::Node *word_node = word_node_set.at(curr_idx);

    if (xmlpp::Element *element = dynamic_cast<xmlpp::Element *>(word_node))
    {
        if (element->has_child_text())
        {
            if (xmlpp::TextNode *text_node = element->get_first_child_text())
            {
                return Word::MakeFromRawStrAndPosTag(
                    text_node->get_content(),
                    element->get_attribute_value("POS"));
            }
        }
    }

    throw ParserException("[WordIterator::operator*] Failed to parse word node");
    return Word::MakeFromRawStrAndPosTag("", "x");
}

bool WordIterator::operator!=(const WordIterator &rhs) const
{
    return get_curr_idx() != rhs.get_curr_idx();
}
