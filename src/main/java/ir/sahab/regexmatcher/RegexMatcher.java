package ir.sahab.regexmatcher;

import ir.sahab.regexmatcher.exception.PatternPreparationException;
import java.io.BufferedInputStream;
import java.io.IOException;
import java.net.URL;
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
import org.apache.commons.compress.archivers.ArchiveEntry;
import org.apache.commons.compress.archivers.tar.TarArchiveInputStream;
import org.apache.commons.compress.compressors.gzip.GzipCompressorInputStream;
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
     * Following the JNI standard, the static block loads the native library once for the class. In order to be able to
     * differentiate between different instances of this class a native instance is created per java instance. This
     * native instance is managed by the native code and its ID will be passed as an argument to methods that call into
     * native code.
     */
    static {
        // System.load cannot load libraries within a jar. So we have to extract it to a
        // temporary location and load it from there.
        Logger logger = LoggerFactory.getLogger(RegexMatcher.class);
        logger.info("Extracting regex matcher JNI library for loading ...");

        URL nativeLibraryResource = RegexMatcher.class.getClassLoader().getResource("lib_jni.tar.gz");
        if (nativeLibraryResource == null) {
            throw new AssertionError("Unable to find JNI library (lib_jni.tar.gz) in resource folder");
        }

        final Path libraryPath;
        try (BufferedInputStream libraryStream = new BufferedInputStream(nativeLibraryResource.openStream());
                GzipCompressorInputStream libraryGzipStream = new GzipCompressorInputStream(libraryStream);
                TarArchiveInputStream libraryTarStream = new TarArchiveInputStream(libraryGzipStream)) {
            ArchiveEntry entry = libraryTarStream.getNextEntry();
            if (!entry.getName().equals("lib_jni.so") || entry.isDirectory()) {
                throw new AssertionError("We expect JNI library (lib_jni.so) in archive file");
            }

            FileAttribute<Set<PosixFilePermission>> permissions = PosixFilePermissions.asFileAttribute(
                    new HashSet<>(Collections.singletonList(PosixFilePermission.OWNER_READ)));
            libraryPath = Files.createTempFile("lib_jni", ".so", permissions).toAbsolutePath();
            Files.copy(libraryTarStream, libraryPath, StandardCopyOption.REPLACE_EXISTING);

            entry = libraryTarStream.getNextEntry();
            if (entry != null) {
                throw new AssertionError("We expect only JNI library (lib_jni.so) in archive file");
            }
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
     * Each instance of the class allocates native resources. A instance ID to access these resources
     * is stored in the following variable. This instance ID is passed as an argument to methods that
     * call into native code.
     */
    private final long instanceId;

    public RegexMatcher() {
        instanceId = newInstance();
    }

    /**
     * Adds a new regex to the engine. Errors in pattern will not be detected immediately, because
     * the pattern is not validated here. Rather it is validated during {@link #preparePatterns()}.
     *
     * @param patternId a positive number identifying the pattern. Multiple patterns may share
     *        the same pattern id.
     * @param pattern the pattern must conform to PCRE syntax: http://www.pcre.org/.
     * @param isCaseSensitive whether the pattern is case-sensitive. This parameter has less
     *        precedence than inline regex flags. e.g. the following pattern is
     *        case-sensitive: <code>addPattern(1, "(?-i)abcd", false)</code>.
     * @see PatternMatcher#addPattern(long, String, boolean)
     */
    @Override
    public void addPattern(long patternId, String pattern, boolean isCaseSensitive) {
        addPattern(instanceId, patternId, pattern, isCaseSensitive);
    }

    private native void addPattern(long instanceId, long patternId, String pattern, boolean isCaseSensitive);

    @Override
    public boolean removePattern(long patternId) {
        return removePattern(instanceId, patternId);
    }

    private native boolean removePattern(long instanceId, long patternId);

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
        preparePatterns(instanceId);
    }

    private native void preparePatterns(long instanceId) throws PatternPreparationException;

    @Override
    public Set<Long> match(String input) {
        return new HashSet<>(match(instanceId, input));
    }

    private native List<Long> match(long instanceId, String input);

    /**
     * Release native resources being held by the object.<br/>
     * After calling this function the object is effectively destroyed, so no further method
     * calls are allowed and a new instance must be created.
     */
    @Override
    public void close() {
        close(instanceId);
    }

    private native void close(long instanceId);

    /**
     * Initializes a new instance of regular expression matching engine and returns an instance ID that
     * will be used in other native method calls.
     */
    private native long newInstance();
}