#ifndef PARSER_HH
#define PARSER_HH

#include <iostream>
#include <list>
#include <string>
#include <utils.hh>
#include <vector>

// Simple inline markdown parser, intended to be translated to Java
// in the future so it can be integrated into Minecraft. That also
// explains the use of e.g. std::list.
struct Parser {
    struct Span;
    struct Emph;
    using Node = std::variant<Emph, Span>;
    using NodeRef = std::list<Node>::iterator;

    struct Emph {
        enum struct Kind {
            Bold,
            Italic,
            Underline,
            Strikethrough
        };

        std::list<Node> nodes;
        Kind kind;
    };

    struct Span {
        u32 start{};
        u32 end{};
        bool is_code = false;

        Span() = default;
        Span(usz start, usz end, bool code = false)
            : start{u32(start)}, end{u32(end)}, is_code{code} {}

        u32 size() const { return end - start; }
    };

    struct Delimiter {
        NodeRef node;
        bool can_open          : 1 = false;
        bool can_close         : 1 = false;
        bool can_open_strong   : 1 = false;
        bool can_close_strong  : 1 = false;
        bool preceded_by_punct : 1 = false;
        bool followed_by_punct : 1 = false;

        bool clopen() { return can_open and can_close; }
        u32 count() { return std::get<Span>(*node).size(); }
        char kind(Parser* p) { return p->input[std::get<Span>(*node).start]; }
        void remove(u32 count) { std::get<Span>(*node).start += count; }
    };

    static constexpr i32 BottomOfStack = -1;

    std::string_view input;
    std::list<Node> nodes;
    std::list<Delimiter> delimiter_stack;

    Parser(std::string_view text);
    bool ClassifyDelimiter(u32 start_of_text, Span text);
    void DumpNodes();
    bool IsUnicodeWhitespace(char c);
    void Parse();
    void ProcessEmphasis();
    auto Print() -> std::string;
};

template <>
struct fmt::formatter<Parser::Emph::Kind> : formatter<std::string_view> {
    template <typename FormatContext>
    auto format(Parser::Emph::Kind k, FormatContext& ctx) {
        using enum Parser::Emph::Kind;
        std::string_view sv = k == Italic    ? "em"
                            : k == Bold      ? "strong"
                            : k == Underline ? "uline"
                                             : "del";
        return formatter<std::string_view>::format(sv, ctx);
    }
};

inline Parser::Parser(std::string_view text) {
    input = text;
    delimiter_stack.emplace_back(); // Bottom of stack.
    Parse();
    DumpNodes();
    ProcessEmphasis();
}

inline bool Parser::ClassifyDelimiter(u32 start_of_text, Span text) {
    // 6.2 Emphasis and strong emphasis
    //
    // A left-flanking delimiter run is a delimiter run that is
    //
    //   (1) not followed by Unicode whitespace,
    //
    //   (2) and either
    //
    //       (2a) not followed by a Unicode punctuation character, or
    //
    //       (2b) followed by a Unicode punctuation character and preceded
    //            by Unicode whitespace or a Unicode punctuation character.
    //
    //   (*) For purposes of this definition, the beginning and the end of the
    //       line count as Unicode whitespace.
    //
    // -----
    //
    // In more intelligible terms, this means that:
    //
    //   1. If the delimiter is at end of text, return FALSE.
    //   2. If the next character is whitespace, return FALSE.
    //   3. If the next character is NOT punctuation, return TRUE.
    //   4. If the delimiter is at start of text, return TRUE.
    //   5. If the previous character is whitespace, return TRUE.
    //   6. If the previous character is punctuation, return TRUE.
    //   7. Otherwise, return FALSE.
    //
    // The same applies to right-flanking, but replace every occurrence of ‘next’
    // with ‘previous’ and vice versa. The algorithm implemented here is the one
    // for left-flanking delimiters. It can be used to compute the right-flanking
    // property by swapping the ‘next’ and ‘prev’ parameters.
    const auto IsFlankingDelimiter = [&](char prev, char next) {
        if (next == 0) return false;             // 1.
        if (std::isspace(next)) return false;    // 2.
        if (not std::ispunct(next)) return true; // 3.
        if (prev == 0) return true;              // 4.
        if (std::isspace(prev)) return true;     // 5.
        if (std::ispunct(prev)) return true;     // 6.
        return false;                            // 7.
    };

    const char next = text.end < input.size() ? input[text.end] : 0;
    const char prev = text.start > 0 ? input[text.start - 1] : 0;
    const char kind = input[text.start];
    bool left_flanking = IsFlankingDelimiter(prev, next);
    bool right_flanking = IsFlankingDelimiter(next, prev);
    bool preceded_by_punct = prev != 0 and std::ispunct(prev);
    bool followed_by_punct = next != 0 and std::ispunct(next);

    // Ibid.
    //
    // Rules rearranged slightly.
    //
    // 1. `*`/`**`
    //   1a. can open (strong) emphasis iff it is part of a left-flanking delimiter run, and
    //   1b. can close (strong) emphasis iff it is part of a right-flanking delimiter run.
    //
    // EXTENSION: `~~` behaves like `**`.
    bool can_open;
    bool can_close;
    if (kind == '*' or kind == '~') {
        can_open = left_flanking;
        can_close = right_flanking;
    }

    // 2. `_`/`__` can open/close (strong) emphasis iff
    //
    //   2a. it is part of a left/right-flanking delimiter run,
    //
    //   2b. and either
    //
    //      2bα. not part of a right/left-flanking delimiter run or
    //
    //      2bβ. part of a right/left-flanking delimiter run preceded/followed
    //           by a Unicode punctuation character.
    else {
        can_open = left_flanking and (not right_flanking or preceded_by_punct);
        can_close = right_flanking and (not left_flanking or followed_by_punct);
    }

    // If this can open or close, then it’s actually a delimiter; otherwise,
    // it’s just text.
    if (not can_open and not can_close) return false;

    // We have a delimiter; append the text we've read so far.
    if (start_of_text != text.start) nodes.emplace_back(Span{start_of_text, text.start});

    // Then, create the delimiter.
    nodes.emplace_back(text);
    auto& delim = delimiter_stack.emplace_back(std::prev(nodes.end()));
    delim.preceded_by_punct = preceded_by_punct;
    delim.followed_by_punct = followed_by_punct;
    delim.can_open = can_open;
    delim.can_close = can_close;

    // Rules for opening strong emphasis are the same as those for opening emphasis,
    // so long as we have at least 2 delimiters in the run.
    if (delim.count() > 1) {
        delim.can_open_strong = delim.can_open;
        delim.can_close_strong = delim.can_close;
    }

    return true;
}

inline void Parser::DumpNodes() {
    struct Printer {
        Parser* p;
        usz indent = 0;
        void operator()(Span sp) {
            fmt::print("{}Span: '{}'\n", std::string(indent, ' '), p->input.substr(sp.start, sp.size()));
        }

        void operator()(Emph& e) {
            fmt::print("{}Emph {}:\n", std::string(indent, ' '), e.kind);
            indent++;
            for (auto& n : e.nodes) std::visit(*this, n);
            indent--;
        }
    };

    Printer p{this};
    for (auto& n : nodes) std::visit(p, n);
}

inline void Parser::Parse() {
    usz pos = 0;
    usz start_of_text = pos;
    while (pos < input.size()) {
        // 6.1 Code spans
        //
        // A backtick string is a string of one or more backtick characters
        // (`) that is neither preceded nor followed by a backtick.
        //
        // 6.2 Emphasis and strong emphasis
        //
        // A delimiter run is either
        //
        //    (1) a sequence of one or more `*` characters that is not preceded or
        //        followed by a non-backslash-escaped `*` character, or
        //
        //    (2) a sequence of one or more `_` characters that is not preceded or
        //        followed by a non-backslash-escaped `_` character.
        //
        // EXTENSION: `~~` is also a delimiter.
        usz start = input.find_first_of("*_~`", pos);
        if (start == std::string::npos) {
            nodes.emplace_back(Span{start_of_text, input.size()});
            return;
        }

        // Check if this is escaped; to do that, read backslashes before the character;
        // note that backslashes can escape each other, so only treat this as escaped
        // if we find an odd number of backslashes.
        usz backslashes = 0;
        while (start - backslashes > 0 and input[start - backslashes - 1] == '\\')
            ++backslashes;
        if (backslashes & 1) {
            pos = start + 1;
            continue;
        }

        // Read the rest of the delimiter.
        usz count = 1;
        while (start + count < input.size() and input[start + count] == input[start])
            ++count;

        // Handle backticks first; unlike emphasis, they are very straight-forward;
        // simply read ahead until we find a corresponding number of backticks (note
        // that backslash-escapes are not allowed in code spans, so we don’t even
        // have to worry about that).
        if (input[start] == '`') {
            auto search_start = start + count;
            for (;;) {
                auto end = input.find({input.data() + start, count}, search_start);

                // If we don’t find a matching backtick string, then these backticks are
                // literal; stop searching.
                if (end == std::string::npos) {
                    // Only skip past the initial backticks.
                    pos = start + count;
                    break;
                }

                // On the other hand, if there are extra backticks here, then this is a
                // longer backtick string, which doesn’t match the one we’re looking for.
                //
                // This handles the case of e.g.: ‘` `` `’, which is ‘<code>``</code>’.
                if (end + count < input.size() and input[end + count] == '`') {
                    // As an optimisation, just skip past all backticks here since we
                    // know that this backtick string isn’t the end anyway.
                    usz c = count;
                    do c++;
                    while (end + c < input.size() and input[end + c] == '`');
                    search_start = end + c;
                    continue;
                }

                // Otherwise, we’ve found the end of a code span.
                nodes.emplace_back(Span{start_of_text, start});
                nodes.emplace_back(Span{start + count, end, true});
                pos = start_of_text = end + count;
                break;
            }

            // We handled the code span, either by skipping it and treating it as literal
            // or by inserting a code span node. In either case, there is no more delimiter
            // processing to be done here.
            continue;
        }

        // EXTENSION: A single `~` is not a delimiter.
        if (input[start] == '~' and count == 1) {
            pos = start + 1;
            continue;
        }

        // Create the delimiter.
        if (ClassifyDelimiter(start_of_text, Span{start, start + count}))
            start_of_text = start + count;

        // Move past it.
        pos = start + count;
    }

    // Append remaining text.
    if (start_of_text < input.size())
        nodes.emplace_back(Span{start_of_text, input.size()});
}

inline void Parser::ProcessEmphasis() {
    // Note: we always have an empty delimiter at the bottom of the stack.
    if (delimiter_stack.size() == 1)
        return;

    // Let current_position point to the element on the delimiter stack
    // just above stack_bottom (or the first element if stack_bottom is NULL).
    auto current_position = std::next(delimiter_stack.begin());

    // We keep track of the openers_bottom for each delimiter type (*, _),
    // indexed to the length of the closing delimiter run (modulo 3) and to
    // whether the closing delimiter can also be an opener. Initialize this
    // to stack_bottom.
    class Openers {
        using Iterator = std::list<Delimiter>::iterator;
        Parser& p;
        std::array<Iterator, 3> star;
        std::array<Iterator, 3> underscore;

    public:
        Openers(Parser& p) : p{p} {
            star.fill(p.delimiter_stack.begin());
            underscore.fill(p.delimiter_stack.begin());
        }

        auto operator[](Delimiter& d) -> Iterator& {
            return d.kind(&p) == '*' ? star[d.count() % 3] : underscore[d.count() % 3];
        }
    } openers{*this};

    // Then we repeat the following until we run out of potential closers:
    for (;;) {
        // Move current_position forward in the delimiter stack (if needed) until
        // we find the first potential closer with delimiter * or _. (This will
        // be the potential closer closest to the beginning of the input – the
        // first one in parse order.)
        //
        // Note: can_close_strong implies can_close, so we only need to check
        // for the latter.
        while (current_position != delimiter_stack.end() and !current_position->can_close)
            ++current_position;

        // Out of closers.
        if (current_position == delimiter_stack.end())
            return;

        // Now, look back in the stack (staying above stack_bottom and the openers_bottom
        // for this delimiter type) for the first matching potential opener (“matching” means
        // same delimiter—that is, same kind *and same count*).
        auto opener = std::prev(current_position);
        bool found = false;
        while (opener != delimiter_stack.begin() and opener != openers[*current_position]) {
            // 6.2 Emphasis and strong emphasis Rule 9/10
            //
            // If one of the delimiters can both open and close emphasis, then the sum of the
            // lengths of the delimiter runs containing the opening and closing delimiters must
            // not be a multiple of 3 unless both lengths are multiples of 3.
            found = [&] {
                if (opener->kind(this) != current_position->kind(this)) return false;
                if (not opener->clopen() and not current_position->clopen()) return true;
                auto l1 = opener->count();
                auto l2 = current_position->count();
                if (l1 % 3 == 0 and l2 % 3 == 0) return true;
                return (l1 + l2) % 3 != 0;
            }();

            if (found) break;
            --opener;
        }

        // If one is found:
        if (found) {
            // Figure out whether we have emphasis or strong emphasis: if both closer and
            // opener spans have length >= 2, we have strong, otherwise regular.
            //
            // EXTENSION: Two __ are underlining instead of strong emphasis.
            bool strong = opener->count() >= 2 and current_position->count() >= 2;
            auto kind = [&] {
                switch (opener->kind(this)) {
                    case '*': return strong ? Emph::Kind::Bold : Emph::Kind::Italic;
                    case '_': return strong ? Emph::Kind::Underline : Emph::Kind::Italic;
                    case '~': return Emph::Kind::Strikethrough; // Always strong.
                    default: die("unreachable");
                }
            }();

            // Insert an emph or strong emph node accordingly, after the text node
            // corresponding to the opener and remove any delimiters between the
            // opener and closer from the delimiter stack.
            auto emph_node = nodes.insert(std::next(opener->node), Emph{{}, kind});
            auto& emph = std::get<Emph>(*emph_node);
            emph.nodes.splice(emph.nodes.begin(), nodes, std::next(emph_node), current_position->node);
            delimiter_stack.erase(std::next(opener), current_position);

            // Remove 1 (for regular emph) or 2 (for strong emph) delimiters from the
            // opening and closing text nodes.
            u8 count = strong ? 2 : 1;
            opener->remove(count);
            current_position->remove(count);

            // If they become empty as a result, remove them and remove the corresponding
            // element of the delimiter stack.
            if (opener->count() == 0) {
                nodes.erase(opener->node);
                delimiter_stack.erase(opener);
            }

            // If the closing node is removed, reset current_position to the next element
            // in the stack.
            if (current_position->count() == 0) {
                nodes.erase(current_position->node);
                current_position = delimiter_stack.erase(current_position);
            }
        }

        // If none is found:
        else {
            // Set openers_bottom to the element before current_position. (We know that
            // there are no openers for this kind of closer up to and including this point,
            // so this puts a lower bound on future searches.)
            openers[*current_position] = std::prev(current_position);

            // If the closer at current_position is not a potential opener, remove it from the
            // delimiter stack (since we know it can’t be a closer either). Advance current_position
            // to the next element in the stack.
            if (!current_position->can_open) current_position = delimiter_stack.erase(current_position);
            else ++current_position;
        }
    }
}

inline auto Parser::Print() -> std::string {
    struct Printer {
        Parser* p;
        std::string s{};
        void operator()(const Span& sp) {
            // Apply normalisation to code spans.
            if (sp.is_code) {
                // First, line endings are converted to spaces.
                auto normalised = std::string{p->input.substr(sp.start, sp.size())};
                for (auto& c : normalised)
                    if (c == '\n')
                        c = ' ';

                // If the resulting string both begins and ends with a space character,
                // but does not consist entirely of space characters, a single space
                // character is removed from the front and back. This allows you to include
                // code that begins or ends with backtick characters, which must be separated
                // by whitespace from the opening or closing backtick strings.
                if (
                    normalised.size() > 2 and
                    normalised.front() == ' ' and
                    normalised.back() == ' ' and
                    normalised.find_first_not_of(' ') != std::string::npos
                ) {
                    normalised.erase(normalised.begin());
                    normalised.pop_back();
                }

                // That’s all for code spans.
                s += fmt::format("<code>{}</code>", normalised);
                return;
            }

            // Regular text; process escapes.
            static constexpr std::string_view escapable = "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~";
            std::string_view text = p->input.substr(sp.start, sp.size());
            usz pos = 0;
            usz start_of_text = 0;
            for (;;) {
                auto backslash = text.find('\\', pos);
                if (backslash == std::string::npos or backslash == text.size() - 1) {
                    s += text.substr(start_of_text);
                    return;
                }

                // 2.4 Backslash escapes
                //
                // Any ASCII punctuation character may be backslash-escaped:
                // Backslashes before other characters are treated as literal backslashes
                char escaped = text[backslash + 1];
                if (escapable.find(escaped) != std::string::npos) {
                    s += text.substr(start_of_text, backslash - start_of_text);
                    s += escaped;
                    start_of_text = backslash + 2;
                }

                // Skip the backslash.
                pos = backslash + 2;
            }
        }

        void operator()(const Emph& e) {
            s += fmt::format("<{}>", e.kind);
            for (auto& n : e.nodes) std::visit(*this, n);
            s += fmt::format("</{}>", e.kind);
        }
    };

    Printer p{this};
    for (auto& n : nodes) std::visit(p, n);
    return p.s;
}


#endif //PARSER_HH
