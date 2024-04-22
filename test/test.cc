#include <parser.hh>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Catch::literals;

auto P(std::string_view input) -> std::string {
    return Parser(input).Print();
}

#define T(input, expected) CHECK(P(input) == expected)

TEST_CASE("Empty string doesn’t crash") {
    T("", "");
}

TEST_CASE("2.4 Backslash escapes") {
    T(
        "\\!\\\"\\#\\$\\%\\&\\'\\(\\)\\*\\+\\,\\-\\.\\/\\:\\;\\<\\=\\>\\?\\@\\[\\\\\\]\\^\\_\\`\\{\\|\\}\\~",
        "!\"#$%&'()*+,-./:;<=>?@[\\]^_`{|}~"
    );

    T("\\→\\A\\a\\ \\3\\φ\\«", "\\→\\A\\a\\ \\3\\φ\\«");
}

TEST_CASE("6.2 Code spans") {
    T("`foo`", "<code>foo</code>");
    T("`` foo ` bar ``", "<code>foo ` bar</code>");
    T("` `` `", "<code>``</code>");
    T("` a`", "<code> a</code>");
    T("`\tb\t`", "<code>\tb\t</code>");
    T("` `\n`  `", "<code> </code>\n<code>  </code>");
    T("``\nfoo\nbar  \nbaz\n``", "<code>foo bar   baz</code>");
    T("``\nfoo \n``", "<code>foo </code>");
    T("`foo   bar \nbaz`", "<code>foo   bar  baz</code>");
    T("`foo\\`bar`", "<code>foo\\</code>bar`");
    T("``foo`bar``", "<code>foo`bar</code>");
    T("` foo `` bar `", "<code>foo `` bar</code>");
    T("*foo`*`", "*foo<code>*</code>");
    T("```foo``", "```foo``");
    T("`foo", "`foo");
    T("`foo``bar``", "`foo<code>bar</code>");

    SECTION("No stripping of spaces if the code span only consists of spaces") {
        T("`    `", "<code>    </code>");
    }
}

TEST_CASE("6.2 Rule 1") {
    T("*foo bar*", "<em>foo bar</em>");
    T("a * foo bar*", "a * foo bar*");
    T("a*\"foo\"*", "a*\"foo\"*");
    T("foo*bar*", "foo<em>bar</em>");
    T("5*6*78", "5<em>6</em>78");
}

TEST_CASE("6.2 Rule 2") {
    T("_foo bar_", "<em>foo bar</em>");
    T("_ foo bar_", "_ foo bar_");
    T("a_\"foo\"_", "a_\"foo\"_");
    T("foo_bar_", "foo_bar_");
    T("5_6_78", "5_6_78");
    T("пристаням_стремятся_", "пристаням_стремятся_");
    T("aa_\"bb\"_cc", "aa_\"bb\"_cc");
    T("foo-_(bar)_", "foo-<em>(bar)</em>");
}

TEST_CASE("6.2 Rule 3") {
    T("_foo*", "_foo*");
    T("*foo bar *", "*foo bar *");
    T("*foo bar\n*", "*foo bar\n*");
    T("*(*foo)", "*(*foo)");
    T("*(*foo*)*", "<em>(<em>foo</em>)</em>");
    T("*foo*bar", "<em>foo</em>bar");
}

TEST_CASE("6.2 Rule 4") {
    T("_foo bar _", "_foo bar _");
    T("_(_foo)", "_(_foo)");
    T("_(_foo_)_", "<em>(<em>foo</em>)</em>");
    T("_foo_bar", "_foo_bar");
    T("_пристаням_стремятся", "_пристаням_стремятся");
    T("_foo_bar_baz_", "<em>foo_bar_baz</em>");
    T("_(bar)_.", "<em>(bar)</em>.");
}

TEST_CASE("6.2 Rule 5") {
    T("**foo bar**", "<strong>foo bar</strong>");
    T("** foo bar**", "** foo bar**");
    T("a**\"foo\"**", "a**\"foo\"**");
    T("foo**bar**", "foo<strong>bar</strong>");
}

TEST_CASE("6.2 Rule 6") {
    T("__foo bar__", "<uline>foo bar</uline>");
    T("__ foo bar__", "__ foo bar__");
    T("__\nfoo bar__", "__\nfoo bar__");
    T("a__\"foo\"__", "a__\"foo\"__");
    T("foo__bar__", "foo__bar__");
    T("5__6__78", "5__6__78");
    T("пристаням__стремятся__", "пристаням__стремятся__");
    T("__foo, __bar__, baz__", "<uline>foo, <uline>bar</uline>, baz</uline>");
    T("foo-__(bar)__", "foo-<uline>(bar)</uline>");
}

TEST_CASE("6.2 Rule 7") {
    T("**foo bar **", "**foo bar **");
    T("**(**foo)", "**(**foo)");
    T("*(**foo**)*", "<em>(<strong>foo</strong>)</em>");
    T("**foo \"*bar*\" foo**", "<strong>foo \"<em>bar</em>\" foo</strong>");
    T("**foo**bar", "<strong>foo</strong>bar");
    T(
        "**Gomphocarpus (*Gomphocarpus physocarpus*, syn.\n*Asclepias physocarpa*)**",
        "<strong>Gomphocarpus (<em>Gomphocarpus physocarpus</em>, syn.\n<em>Asclepias physocarpa</em>)</strong>"
    );
}

TEST_CASE("6.2 Rule 8") {
    T("__foo bar __", "__foo bar __");
    T("__(__foo)", "__(__foo)");
    T("_(__foo__)_", "<em>(<uline>foo</uline>)</em>");
    T("__foo__bar", "__foo__bar");
    T("__пристаням__стремятся", "__пристаням__стремятся");
    T("__foo__bar__baz__", "<uline>foo__bar__baz</uline>");
    T("__(bar)__.", "<uline>(bar)</uline>.");
}

TEST_CASE("6.2 Rule 9") {
    T("*foo\nbar*", "<em>foo\nbar</em>");
    T("_foo __bar__ baz_", "<em>foo <uline>bar</uline> baz</em>");
    T("_foo _bar_ baz_", "<em>foo <em>bar</em> baz</em>");
    T("__foo_ bar_", "<em><em>foo</em> bar</em>");
    T("*foo *bar**", "<em>foo <em>bar</em></em>");
    T("*foo **bar** baz*", "<em>foo <strong>bar</strong> baz</em>");
    T("*foo**bar**baz*", "<em>foo<strong>bar</strong>baz</em>");
    T("*foo**bar*", "<em>foo**bar</em>");
    T("***foo** bar*", "<em><strong>foo</strong> bar</em>");
    T("*foo **bar***", "<em>foo <strong>bar</strong></em>");
    T("*foo**bar***", "<em>foo<strong>bar</strong></em>");
    T("foo***bar***baz", "foo<em><strong>bar</strong></em>baz");
    T("foo******bar*********baz", "foo<strong><strong><strong>bar</strong></strong></strong>***baz");
    T("*foo **bar *baz* bim** bop*", "<em>foo <strong>bar <em>baz</em> bim</strong> bop</em>");
    T("** is not an empty emphasis", "** is not an empty emphasis");
    T("**** is not an empty strong emphasis", "**** is not an empty strong emphasis");
}

TEST_CASE("6.2 Rule 10") {
    T("**foo\nbar**", "<strong>foo\nbar</strong>");
    T("__foo _bar_ baz__", "<uline>foo <em>bar</em> baz</uline>");
    T("__foo __bar__ baz__", "<uline>foo <uline>bar</uline> baz</uline>");
    T("____foo__ bar__", "<uline><uline>foo</uline> bar</uline>");
    T("**foo **bar****", "<strong>foo <strong>bar</strong></strong>");
    T("**foo *bar* baz**", "<strong>foo <em>bar</em> baz</strong>");
    T("**foo*bar*baz**", "<strong>foo<em>bar</em>baz</strong>");
    T("***foo* bar**", "<strong><em>foo</em> bar</strong>");
    T("**foo *bar***", "<strong>foo <em>bar</em></strong>");
    T("**foo *bar **baz**\nbim* bop**", "<strong>foo <em>bar <strong>baz</strong>\nbim</em> bop</strong>");
    T("__ is not an empty emphasis", "__ is not an empty emphasis");
    T("____ is not an empty strong emphasis", "____ is not an empty strong emphasis");

}

TEST_CASE("6.2 Rule 11") {
    T("foo ***", "foo ***");
    T("foo *\\**", "foo <em>*</em>");
    T("foo *_*", "foo <em>_</em>");
    T("foo *****", "foo *****");
    T("foo **\\***", "foo <strong>*</strong>");
    T("foo **_**", "foo <strong>_</strong>");
    T("**foo*", "*<em>foo</em>");
    T("*foo**", "<em>foo</em>*");
    T("***foo**", "*<strong>foo</strong>");
    T("****foo*", "***<em>foo</em>");
    T("**foo***", "<strong>foo</strong>*");
    T("*foo****", "<em>foo</em>***");
}

TEST_CASE("6.2 Rule 12") {
    T("foo ___", "foo ___");
    T("foo _\\__", "foo <em>_</em>");
    T("foo _*_", "foo <em>*</em>");
    T("foo _____", "foo _____");
    T("foo __\\___", "foo <uline>_</uline>");
    T("foo __*__", "foo <uline>*</uline>");
    T("__foo_", "_<em>foo</em>");
    T("_foo__", "<em>foo</em>_");
    T("___foo__", "_<uline>foo</uline>");
    T("____foo_", "___<em>foo</em>");
    T("__foo___", "<uline>foo</uline>_");
    T("_foo____", "<em>foo</em>___");
}

TEST_CASE("6.2 Rule 13") {
    T("**foo**", "<strong>foo</strong>");
    T("*_foo_*", "<em><em>foo</em></em>");
    T("__foo__", "<uline>foo</uline>");
    T("_*foo*_", "<em><em>foo</em></em>");
    T("****foo****", "<strong><strong>foo</strong></strong>");
    T("____foo____", "<uline><uline>foo</uline></uline>");
    T("******foo******", "<strong><strong><strong>foo</strong></strong></strong>");
}

TEST_CASE("6.2 Rule 14") {
    T("***foo***", "<em><strong>foo</strong></em>");
    T("_____foo_____", "<em><uline><uline>foo</uline></uline></em>");
}

TEST_CASE("6.2 Rule 15") {
    T("*foo _bar* baz_", "<em>foo _bar</em> baz_");
    T("*foo __bar *baz bim__ bam*", "<em>foo <uline>bar *baz bim</uline> bam</em>");
}

TEST_CASE("6.2 Rule 16") {
    T("**foo **bar baz**", "**foo <strong>bar baz</strong>");
    T("*foo *bar baz*", "*foo <em>bar baz</em>");
}

TEST_CASE("Strikethrough extension") {
    T("~~", "~~");
    T("~~t~~", "<del>t</del>");
    T("~~t~~t~~", "<del>t</del>t~~");
    T("~t~~", "~t~~");
    T("~~t~", "~~t~");
    T("~~t", "~~t");
    T("~~~~ t", "~~~~ t");
    T("**foo ~~bar~~ baz**", "<strong>foo <del>bar</del> baz</strong>");
    T("~~foo\nbar~~", "<del>foo\nbar</del>");
    T("~~t~\\~e\\~~v~~", "<del>t~~e~~v</del>");
}

TEST_CASE("Spoiler extension") {
    T("||", "||");
    T("||t||", "<spoiler>t</spoiler>");
    T("||t||t||", "<spoiler>t</spoiler>t||");
    T("|t||", "|t||");
    T("||t|", "||t|");
    T("||t", "||t");
    T("|||| t", "|||| t");
    T("**foo ||bar|| baz**", "<strong>foo <spoiler>bar</spoiler> baz</strong>");
    T("||foo\nbar||", "<spoiler>foo\nbar</spoiler>");
    T("||t|\\|e\\||v||", "<spoiler>t||e||v</spoiler>");
}