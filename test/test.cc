#include <parser.hh>
#include <catch2/catch_all.hpp>
#include <catch2/catch_test_macros.hpp>

using namespace Catch::literals;

auto P(std::string_view input) -> std::string {
    return Parser(input).Print();
}

#define T(input, expected) CHECK(P(input) == expected)

TEST_CASE("Commonmark Spec Tests") {
    SECTION("Rule 1") {
        T("*foo bar*", "<em>foo bar</em>");
        T("a * foo bar*", "a * foo bar*");
        T("a*\"foo\"*", "a*\"foo\"*");
        T("foo*bar*", "foo<em>bar</em>");
        T("5*6*78", "5<em>6</em>78");
    }

    SECTION("Rule 2") {
        T("_foo bar_", "<em>foo bar</em>");
        T("_ foo bar_", "_ foo bar_");
        T("a_\"foo\"_", "a_\"foo\"_");
        T("foo_bar_", "foo_bar_");
        T("5_6_78", "5_6_78");
        T("пристаням_стремятся_", "пристаням_стремятся_");
        T("aa_\"bb\"_cc", "aa_\"bb\"_cc");
        T("foo-_(bar)_", "foo-<em>(bar)</em>");
    }

    SECTION("Rule 3") {
        T("_foo*", "_foo*");
        T("*foo bar *", "*foo bar *");
        T("*foo bar\n*", "*foo bar\n*");
        T("*(*foo)", "*(*foo)");
        T("*(*foo*)*", "<em>(<em>foo</em>)</em>");
        T("*foo*bar", "<em>foo</em>bar");
    }

    SECTION("Rule 4") {
        T("_foo bar _", "_foo bar _");
        T("_(_foo)", "_(_foo)");
        T("_(_foo_)_", "<em>(<em>foo</em>)</em>");
        T("_foo_bar", "_foo_bar");
        T("_пристаням_стремятся", "_пристаням_стремятся");
        T("_foo_bar_baz_", "<em>foo_bar_baz</em>");
        T("_(bar)_.", "<em>(bar)</em>.");
    }

    SECTION("Rule 5") {
        T("**foo bar**", "<strong>foo bar</strong>");
        T("** foo bar**", "** foo bar**");
        T("a**\"foo\"**", "a**\"foo\"**");
        T("foo**bar**", "foo<strong>bar</strong>");
    }

    SECTION("Rule 6") {
        T("__foo bar__", "<strong>foo bar</strong>");
        T("__ foo bar__", "__ foo bar__");
        T("__\nfoo bar__", "__\nfoo bar__");
        T("a__\"foo\"__", "a__\"foo\"__");
        T("foo__bar__", "foo__bar__");
        T("5__6__78", "5__6__78");
        T("пристаням__стремятся__", "пристаням__стремятся__");
        T("__foo, __bar__, baz__", "<strong>foo, <strong>bar</strong>, baz</strong>");
        T("foo-__(bar)__", "foo-<strong>(bar)</strong>");
    }

    SECTION("Rule 7") {
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

    SECTION("Rule 8") {
        T("__foo bar __", "__foo bar __");
        T("__(__foo)", "__(__foo)");
        T("_(__foo__)_", "<em>(<strong>foo</strong>)</em>");
        T("__foo__bar", "__foo__bar");
        T("__пристаням__стремятся", "__пристаням__стремятся");
        T("__foo__bar__baz__", "<strong>foo__bar__baz</strong>");
        T("__(bar)__.", "<strong>(bar)</strong>.");
    }

    SECTION("Rule 9") {
        T("*foo\nbar*", "<em>foo\nbar</em>");
        T("_foo __bar__ baz_", "<em>foo <strong>bar</strong> baz</em>");
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

    SECTION("Rule 10") {
        T("**foo\nbar**", "<strong>foo\nbar</strong>");
        T("__foo _bar_ baz__", "<strong>foo <em>bar</em> baz</strong>");
        T("__foo __bar__ baz__", "<strong>foo <strong>bar</strong> baz</strong>");
        T("____foo__ bar__", "<strong><strong>foo</strong> bar</strong>");
        T("**foo **bar****", "<strong>foo <strong>bar</strong></strong>");
        T("**foo *bar* baz**", "<strong>foo <em>bar</em> baz</strong>");
        T("**foo*bar*baz**", "<strong>foo<em>bar</em>baz</strong>");
        T("***foo* bar**", "<strong><em>foo</em> bar</strong>");
        T("**foo *bar***", "<strong>foo <em>bar</em></strong>");
        T("**foo *bar **baz**\nbim* bop**", "<strong>foo <em>bar <strong>baz</strong>\nbim</em> bop</strong>");
        T("__ is not an empty emphasis", "__ is not an empty emphasis");
        T("____ is not an empty strong emphasis", "____ is not an empty strong emphasis");

    }

    SECTION("Rule 11") {
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

    SECTION("Rule 12") {
        T("foo ___", "foo ___");
        T("foo _\\__", "foo <em>_</em>");
        T("foo _*_", "foo <em>*</em>");
        T("foo _____", "foo _____");
        T("foo __\\___", "foo <strong>_</strong>");
        T("foo __*__", "foo <strong>*</strong>");
        T("__foo_", "_<em>foo</em>");
        T("_foo__", "<p><em>foo</em>_</p>");
        T("___foo__", "<p>_<strong>foo</strong></p>");
        T("____foo_", "<p>___<em>foo</em></p>");
        T("__foo___", "<p><strong>foo</strong>_</p>");
        T("_foo____", "<p><em>foo</em>___</p>");
    }

    SECTION("Rule 13") {
        T("**foo**", "<strong>foo</strong></p>");
        T("*_foo_*", "<em><em>foo</em></em></p>");
        T("__foo__", "<strong>foo</strong></p>");
        T("_*foo*_", "<em><em>foo</em></em></p>");
        T("****foo****", "<strong><strong>foo</strong></strong></p>");
        T("____foo____", "<strong><strong>foo</strong></strong></p>");
        T("******foo******", "<strong><strong><strong>foo</strong></strong></strong></p>");
    }

    SECTION("Rule 14") {
        T("***foo***", "<em><strong>foo</strong></em>");
        T("_____foo_____", "<em><strong><strong>foo</strong></strong></em>");
    }

    SECTION("Rule 15") {
        T("*foo _bar* baz_", "<em>foo _bar</em> baz_");
        T("*foo __bar *baz bim__ bam*", "<em>foo <strong>bar *baz bim</strong> bam</em>");
    }

    SECTION("Rule 16") {
        T("**foo **bar baz**", "**foo <strong>bar baz</strong>");
        T("*foo *bar baz*", "*foo <em>bar baz</em>");
    }
}
