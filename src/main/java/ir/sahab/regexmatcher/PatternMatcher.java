package ir.sahab.regexmatcher;

import ir.sahab.regexmatcher.exception.PatternPreparationException;
import java.io.Closeable;
import java.util.Set;

/**
 * Each {@link PatternMatcher} maintains a set of rules that can be modified using
 * {@link #addPattern(long, String, boolean)} and {@link #removePattern(long)} methods. When
 * modifications to pattern set are made, the user <b>must</b> call {@link #preparePatterns()}
 * to ensure required preparation are performed. An example usage scenario would be like the
 * following:
 * <pre>
 *     matcher.addPattern(...);
 *     matcher.removePattern(...);
 *     ...
 *     matcher.preparePatterns();
 *     matchResult = matcher.match(...);
 * </pre>
 *
 * <p>The details about the structure of patterns and how the matching would be done depends on
 * the implementation. In a typical usage, any number of patterns could be added or removed and
 * when {@link #match(String)} is called, only the current existing patterns would be checked
 * against the input argument.
 */
public interface PatternMatcher extends Closeable {

    /**
     * Adds a pattern to the engine.<br/>
     * NOTE: Multiple patterns may be added with the same pattern ID.
     *
     * @param patternId a number identifying the pattern being added.
     * @param pattern a string specifying the pattern to be matched against inputs.
     * @param isCaseSensitive whether the pattern should be matched in a case sensitive/insensitive manner.
     */
    void addPattern(long patternId, String pattern, boolean isCaseSensitive);

    /**
     * Removes all patterns associated with the specified pattern ID.
     *
     * @param patternId the pattern ID to be removed.
     * @return whether or not the pattern existed before removal.
     */
    boolean removePattern(long patternId);

    /**
     * Performs the required preparation when the pattern set has been modified (i.e.
     * {@link #addPattern(long, String, boolean)}/{@link #removePattern(long)} have been called).
     * This method <b>should</b> be called before {@link #match(String)} if any modifications have occurred.
     *
     * @throws PatternPreparationException when preparing the current set of patterns fails.
     */
    void preparePatterns() throws PatternPreparationException;

    /**
     * Compares the input with all patterns currently added to the engine and returns pattern IDs that match.
     *
     * @param input the string that is to be matched against patterns.
     * @return a set of pattern IDs that match the input string.
     */
    Set<Long> match(String input);
}
