#ifndef HYPERSCAN_JAVA_WRAPPER_HYPERSCAN_WRAPPER_H_
#define HYPERSCAN_JAVA_WRAPPER_HYPERSCAN_WRAPPER_H_

#include <map>
#include <string>
#include <set>

#include <ch.h>

namespace sahab {

// A simple OO wrapper of Chimera.
// Chimera is a software regular expression matching engine that is a hybrid of Hyperscan and PCRE.
// The design goals of Chimera are to fully support PCRE syntax as well as to take advantage of the
// high performance nature of Hyperscan.
// An instance of this class is responsible for maintaining a set of patterns that can be updated
// throughout its lifetime. At any point the Match() method can be called to check the input against
// the current set of patterns.
// NOTE: Hyperscan does not support updating and deleting patterns. It only supports building an
// immutable database of known set of patterns. This wrapper allows update/delete at the cost of
// rebuilding the database. So the caller MUST call CompilePatterns() before calling Match()
// whenever the pattern set changes.
// An example usage scenario:
//
//      wrapper.AddPattern(...);
//      wrapper.RemovePattern(...);
//      ...
//      wrapper.CompilePatterns();  // This is required, or subsequent calls to Match() will fail.
//      wrapper.Match(...);
//
// This class is not thread-safe.
class HyperscanWrapper {

  public:
    HyperscanWrapper();
    ~HyperscanWrapper();

    // Adds a new pattern to pattern set.
    // Chimera fully supports the pattern syntax used by the PCRE library (“libpcre”), described
    // at http://www.pcre.org/.
    void AddPattern(unsigned int id, const char* pattern, bool is_case_sensitive);

    // Removes a pattern from pattern set.
    // Returns whether the pattern existed before removal.
    bool RemovePattern(unsigned int id);

    // Compiles the current set of patterns into a Hyperscan database.
    // The caller MUST call this method before calling Match() if the patterns set has changed.
    // Returns 0 on success, < 0 if error is not specific to a pattern, > 0 id of the first
    // erroneous pattern. Call GetLastError() for a string explanation of the error.
    int CompilePatterns();

    // Matches the given string against all patterns in the current pattern set and stores the
    // pattern ids that match in results.
    // Returns false if an error occurs. Call GetLastError() for more information.
    bool Match(const std::string& input, std::set<unsigned int>* results);

    // Returns a string explanation of the last error that has occurred.
    const char* GetLastError() const;

  private:
    void CleanUp();

    // Callback method that will be called when a match occurs.
    void MatchHandler(unsigned int id, unsigned long long from, unsigned long long to, unsigned int flags);

    // Callback function that will be used by Chimera to report matched patterns.
    // The reason that this second function exists is that methods have a hidden parameter this.
    // So we have to define this static method and pass in this as the context parameter so that
    // Chimera can report matches back to the instance.
    static ch_callback_t ScanMatchEventHandler(unsigned int id, unsigned long long from, unsigned long long to,
            unsigned int flags, unsigned int size, const ch_capture_t *captured, void *context);

    // Callback function will be invoked when an error occurs during matching; this indicates that
    // some matches for a given input may not be reported. Currently these errors correspond to
    // resource limits on PCRE backtracking (CH_ERROR_MATCHLIMIT and CH_ERROR_RECURSIONLIMIT).
    static ch_callback_t ScanErrorEventHandler(ch_error_event_t error_type, unsigned int id, void *info, void *context);

    // multimap<id, (pattern, isCaseSensitive)>
    std::multimap<unsigned int, std::pair<std::string, bool> > patterns_;
    std::set<unsigned int> match_results_;
    std::string last_error_;

    bool is_compile_required_;
    ch_database_t* pattern_database_;
    ch_scratch_t* scratch_;
};
}
#endif
