package ir.sahab.regexmatcher.exception;

import ir.sahab.regexmatcher.PatternMatcher;

/**
 * Exception raised by {@link PatternMatcher#preparePatterns()}.
 * Contains an error message along with the ID of the pattern that caused the error, or a
 * negative number if error is not specific to a pattern.
 */
public class PatternPreparationException extends Exception {

    private static final long serialVersionUID = 1L;
    private final long erroneousPatternId;

    public PatternPreparationException(String message, long erroneousPatternId) {
        super(message);
        this.erroneousPatternId = erroneousPatternId;
    }

    public long getErroneousPatternId() {
        return erroneousPatternId;
    }

}
