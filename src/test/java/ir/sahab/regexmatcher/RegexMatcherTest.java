package ir.sahab.regexmatcher;

import static org.junit.Assert.assertEquals;
import static org.junit.Assert.assertFalse;
import static org.junit.Assert.assertTrue;
import static org.junit.Assert.fail;

import ir.sahab.regexmatcher.exception.PatternPreparationException;
import java.io.IOException;
import java.util.Arrays;
import java.util.Collections;
import java.util.HashSet;
import org.junit.After;
import org.junit.Test;

public class RegexMatcherTest {

    private final RegexMatcher matcher = new RegexMatcher();

    @After
    public void tearDown() throws IOException {
        matcher.close();
    }

    @Test
    public void testMatchExactPatterns() throws PatternPreparationException {
        // NOTE: patterns start with ^ and end with $. So a match occurs only when the whole input matches the pattern.
        matcher.addPattern(1L, "^a+$", false);
        matcher.addPattern(2L, "^(a|b)+$", false);
        matcher.preparePatterns();

        assertTrue(matcher.match("").isEmpty());
        assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), matcher.match("a"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("ab"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("b"));
    }

    @Test
    public void testMatchContainsPatterns() throws PatternPreparationException {
        // NOTE: There is no ^ or $. So a match occurs even if only a part of input string matches the pattern.
        matcher.addPattern(1L, "a+", false);
        matcher.addPattern(2L, "(a|b)+", false);
        matcher.preparePatterns();

        assertTrue(matcher.match("").isEmpty());
        assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), matcher.match("a"));
        assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), matcher.match("ab"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("b"));
    }

    @Test
    public void testDeletePattern() throws PatternPreparationException {
        matcher.addPattern(1L, "a+", false);
        // It is legal to have multiple patterns with the same id.
        // {@link RegexMatcher#removePattern(long)} should delete them all.
        matcher.addPattern(1L, "b+", false);
        matcher.addPattern(2L, "c+", false);
        matcher.preparePatterns();

        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("a"));
        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("b"));

        assertTrue(matcher.removePattern(1L));
        matcher.preparePatterns();

        assertTrue(matcher.match("a").isEmpty());
        assertTrue(matcher.match("b").isEmpty());
    }

    @Test
    public void testDeleteNonExistingPattern() {
        assertFalse(matcher.removePattern(1L));
    }

    @Test
    public void testAddDuplicatePatternId() throws PatternPreparationException {
        matcher.addPattern(1L, "a+", false);
        matcher.addPattern(1L, "b+", false);
        matcher.addPattern(1L, "c+", false);
        matcher.preparePatterns();

        assertTrue(matcher.match("a").contains(1L));
        assertTrue(matcher.match("b").contains(1L));
        assertTrue(matcher.match("c").contains(1L));
        assertTrue(matcher.match("x").isEmpty());
    }

    @Test(expected = AssertionError.class)
    public void testAddPatternAfterClose() throws Exception {
        matcher.close();
        matcher.addPattern(1L, "", false);
    }

    @Test(expected = AssertionError.class)
    public void testRemovePatternAfterClose() throws Exception {
        matcher.close();
        matcher.removePattern(1L);
    }

    @Test(expected = AssertionError.class)
    public void testMatchAfterClose() throws Exception {
        matcher.close();
        matcher.match("");
    }

    @Test(expected = IllegalArgumentException.class)
    public void testBigInvalidPatternId() throws Exception {
        matcher.addPattern(Long.MAX_VALUE, "a+", false);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testZeroPatternId() throws Exception {
        matcher.addPattern(0L, "a+", false);
    }

    @Test(expected = IllegalArgumentException.class)
    public void testNegativePatternId() throws Exception {
        matcher.addPattern(-1L, "a+", false);
    }

    @Test
    public void testPatternsWithCharacterClasses() throws PatternPreparationException {
        matcher.addPattern(1L, "^\\w+$", false);
        matcher.addPattern(2L, "^[[:digit:]]+$", false);
        matcher.preparePatterns();

        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("abcd1"));
        assertEquals(new HashSet<>(Arrays.asList(1L, 2L)), matcher.match("121"));
    }

    @Test
    public void testInvalidRegexSyntax() {
        try {
            matcher.addPattern(1000000000L, "ab", false);   // OK
            matcher.addPattern(2000000000L, "(ab", false);  // Missing closing parenthesis.
            matcher.addPattern(3000000000L, "abd", false);  // OK
            matcher.addPattern(4000000000L, // Missing open parenthesis, but only the first error is reported.
                    "abd)", false);
            matcher.preparePatterns();
            fail();
        } catch (PatternPreparationException ex) {
            assertEquals(2000000000L, ex.getErroneousPatternId());
        }
    }

    @Test
    public void testCaseSensitivity() throws PatternPreparationException {
        matcher.addPattern(1L, "pattern", false);  // case insensitive
        matcher.addPattern(2L, "pattern", true);  // case sensitive
        matcher.addPattern(3L, "(?i)pattern", true); // case insensitive because syntax precedes flag
        matcher.addPattern(4L, "(?-i)pattern", false); // case sensitive because syntax precedes flag
        matcher.preparePatterns();

        assertEquals(new HashSet<>(Arrays.asList(1L, 2L, 3L, 4L)), matcher.match("pattern"));
        assertEquals(new HashSet<>(Arrays.asList(1L, 3L)), matcher.match("PATTERN"));
    }

    @Test
    public void testPositiveAndNegativeLookahead() throws PatternPreparationException {
        matcher.addPattern(1L, "^q(?!u)$", false);
        matcher.addPattern(2L, "^q(?!uv)u$", false);
        matcher.addPattern(3L, "^q(?=u)uv$", false);
        matcher.addPattern(4L, "^q(?=u)v$", false); // never match anything
        matcher.preparePatterns();

        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("q"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("qu"));
        assertEquals(new HashSet<>(Collections.singletonList(3L)), matcher.match("quv"));
    }

    @Test
    public void testPositiveAndNegativeLookbehind() throws PatternPreparationException {
        matcher.addPattern(1L, "(?<!a)b", false);
        matcher.addPattern(2L, "(?<=a)b", false);
        matcher.preparePatterns();

        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("bed"));
        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("debt"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("cab"));
    }

    @Test
    public void testAtomicLookaround() throws PatternPreparationException {
        matcher.addPattern(1L, "(?=(\\d+))\\w+\\1", false);
        matcher.preparePatterns();

        assertTrue(matcher.match("123x12").isEmpty());
        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("456x56"));
    }

    @Test
    public void testUtf8Regex() throws PatternPreparationException {
        matcher.addPattern(1L, "^بار$", false);
        // \p{L} any kind of letter from any language.
        // \p{N} any kind of numeric character in any script.
        matcher.addPattern(2L, "(*UTF8)^(\\p{L})+(\\p{N})+$", false);
        matcher.preparePatterns();

        assertTrue(matcher.match("با").isEmpty());
        assertTrue(matcher.match("بارش").isEmpty());
        assertEquals(new HashSet<>(Collections.singletonList(1L)), matcher.match("بار"));
        assertEquals(new HashSet<>(Collections.singletonList(2L)), matcher.match("بار۲"));
        assertTrue(matcher.match("۲ بار").isEmpty());
    }
}