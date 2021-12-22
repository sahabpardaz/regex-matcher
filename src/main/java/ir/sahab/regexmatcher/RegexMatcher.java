package ir.sahab.regexmatcher;

import ir.sahab.regexmatcher.exception.PatternPreparationException;
import java.io.IOException;
import java.io.InputStream;
import java.nio.file.Files;
import java.nio.file.Path;
import java.nio.file.StandardCopyOption;
import java.nio.file.attribute.FileAttribute;
import java.nio.file.attribute.PosixFilePermission;
import java.nio.file.attribute.PosixFilePermissions;
import java.util.Collections;
import java.util.HashSet;
import java.util.List;
import java.util.Set;
import org.slf4j.Logger;
import org.slf4j.LoggerFactory;

/**
 * Pattern matching engine for simultaneously matching multiple regular expressions.
 * This class uses JNI to do the multi-regex matching efficiently with Hyperscan.
 *
 * <p>Note: Instances of this class are not thread-safe.
 *
 * @see PatternMatcher
 */
public class RegexMatcher implements PatternMatcher {

    /*
     * Following the JNI standard, the static block loads the native library once for the class.
     * In order to be able to differentiate between different instances of this class a native
     * handle is created per instance. This handle is managed by the native code and will be
     * passed as an argument to methods that call into native code.
     */
    static {
        // System.load cannot load libraries within a jar. So we have to extract it to a
        // temporary location and load it from there.
        Logger logger = LoggerFactory.getLogger(RegexMatcher.class);
        logger.info("Extracting regex mathcer JNI library for loading ...");

        InputStream nativeLibraryStream = RegexMatcher.class.getClassLoader().getResourceAsStream("lib_jni.so");
        if (nativeLibraryStream == null) {
            throw new AssertionError("Unable to find JNI library (lib_jni.so) in resource folder");
        }

        final Path libraryPath;
        try {
            FileAttribute<Set<PosixFilePermission>> permissions = PosixFilePermissions.asFileAttribute(
                    new HashSet<>(Collections.singletonList(PosixFilePermission.OWNER_READ)));
            libraryPath = Files.createTempFile("lib_jni", ".so", permissions).toAbsolutePath();
            Files.copy(nativeLibraryStream, libraryPath, StandardCopyOption.REPLACE_EXISTING);
        } catch (IOException e) {
            throw new AssertionError("Failed to copy JNI library to a temporary path", e);
        }

        logger.info("Loading JNI library: path = {}", libraryPath);
        System.load(libraryPath.toString());
        logger.info("JNI library loaded successfully: path = {}", libraryPath);

        Runtime.getRuntime().addShutdownHook(new Thread(() -> {
            try {
                Files.delete(libraryPath);
                logger.info("Removed temporarily created JNI library file");
            } catch (IOException e) {
                logger.error("Failed to delete JNI library file: path = " + libraryPath, e);
            }
        }));
    }

    /*
     * Each instance of the class allocates native resources. A handle to access these resources
     * is stored in the following variable. This handle is passed as an argument to methods that
     * call into native code.
     */
    private final long nativeHandle;

    public RegexMatcher() {
        nativeHandle = newInstance();
    }

    /**
     * Adds a new regex the the engine. Errors in pattern will not be detected immendiately, because
     * the pattern is not validated here. Rather it is validated during {@link #preparePatterns()}.
     *
     * @param patternId a positive number identifying the pattern. Multiple patterns may share
     *        the same pattern id.
     * @param pattern the pattern must conform to PCRE syntax: http://www.pcre.org/.
     * @param isCaseSensitive whether the pattern is case sensitive. This parameter has less
     *        precedence than inline regex flags. e.g. the following pattern is
     *        case sensitive: <code>addPattern(1, "(?-i)abcd", false)</code>.
     * @see PatternMatcher#addPattern(long, String, boolean)
     */
    @Override
    public void addPattern(long patternId, String pattern, boolean isCaseSensitive) {
        addPattern(nativeHandle, patternId, pattern, isCaseSensitive);
    }

    private native void addPattern(long instanceHandle, long patternId, String pattern, boolean isCaseSensitive);

    @Override
    public boolean removePattern(long patternId) {
        return removePattern(nativeHandle, patternId);
    }

    private native boolean removePattern(long instanceHandle, long patternId);

    /**
     * Compiles the set of patterns into a database to be used later for matching.<br/>
     * Every time {@link #addPattern(long, String, boolean)}/{@link #removePattern(long)} is
     * called the current database is outdated and {@link #preparePatterns()} <b>must</b> be
     * called. The call to {@link #preparePatterns()} can be delayed until calling
     * {@link #match(String)} (call it just before that).
     *
     * @see PatternMatcher#preparePatterns()
     */
    @Override
    public void preparePatterns() throws PatternPreparationException {
        preparePatterns(nativeHandle);
    }

    private native void preparePatterns(long instanceHandle) throws PatternPreparationException;

    @Override
    public Set<Long> match(String input) {
        return new HashSet<>(match(nativeHandle, input));
    }

    private native List<Long> match(long instanceHandle, String input);

    /**
     * Release native resources being held by the object.<br/>
     * After calling this function the object is effectively destroyed, so no further method
     * calls are allowed and a new instance must be created.
     */
    @Override
    public void close() {
        close(nativeHandle);
    }

    private native void close(long instanceHandle);

    /**
     * Initializes a new instance of regular expression matching engine and returns a handle that
     * will be used in other native method calls.
     */
    private native long newInstance();
}