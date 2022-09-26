#include "hyperscan_wrapper.h"

#include <cstdlib>
#include <vector>

using std::string;
using std::to_string;
using std::vector;
using std::set;
using std::pair;

namespace sahab {

HyperscanWrapper::HyperscanWrapper() :
    is_compile_required_(true),
    pattern_database_(nullptr),
    scratch_(nullptr) {
}

HyperscanWrapper::~HyperscanWrapper() {
    CleanUp();
}

void HyperscanWrapper::AddPattern(unsigned int id, const char* pattern, bool is_case_sensitive) {
    patterns_.insert(pair<unsigned int, pair<string, bool> >(
        id, pair<string, bool>(pattern, is_case_sensitive)));
    is_compile_required_ = true;
}

bool HyperscanWrapper::RemovePattern(unsigned int id) {
    if (0 == patterns_.erase(id)) {
        return false;
    }

    is_compile_required_ = true;
    return true;
}

int64_t HyperscanWrapper::CompilePatterns() {
    if (!is_compile_required_) {
        return 0;
    }
    CleanUp();
    if (patterns_.empty()) {
        is_compile_required_ = false;
        return 0;
    }

    // Patterns stored in patterns_ keep all information about each pattern in one place.
    // Chimera needs this in a different format. The following lines breaks up the information
    // stored in patterns_ elements into separate vectors to be used by Chimera.
    // An alternative would be to keep the information in the format that Chimera requires.
    // The downside of this would be to manage the C strings.
    vector<unsigned int> pattern_ids;
    vector<const char*> cstr_patterns;
    vector<unsigned int> flags;
    unsigned int flag = CH_FLAG_SINGLEMATCH;
    for (const auto& pair : patterns_) {
        pattern_ids.push_back(pair.first);
        cstr_patterns.push_back(pair.second.first.c_str());
        if (pair.second.second) {
            flag &= ~CH_FLAG_CASELESS;
        } else {
            flag |= CH_FLAG_CASELESS;
        }
        flags.push_back(flag);
    }

    ch_compile_error_t* compile_err;
    auto error = ch_compile_multi(cstr_patterns.data(), flags.data(), pattern_ids.data(),
            cstr_patterns.size(), CH_MODE_NOGROUPS, nullptr, &pattern_database_, &compile_err);
    if (error != CH_SUCCESS) {
        auto erroneous_pattern_index = compile_err->expression;

        last_error_ = ("Unable to compile patterns: error = ");
        last_error_.append(compile_err->message);
        ch_free_compile_error(compile_err);
        if (erroneous_pattern_index >= 0) {
            // Convert index to pattern id
            auto erroneous_pattern_id = pattern_ids[erroneous_pattern_index];
            last_error_.append(", erroneous pattern id = ");
            last_error_ += to_string(erroneous_pattern_id);
            return erroneous_pattern_id;
        } else {
            last_error_.append(", unknown expression index = ");
            last_error_ += to_string(erroneous_pattern_index);
            return -1;
        }
    }

    error = ch_alloc_scratch(pattern_database_, &scratch_);
    if (error != CH_SUCCESS) {
        last_error_ = "Unable to allocate scratch: error = " + to_string(error);
        return -1;
    }

    is_compile_required_ = false;
    return 0;
}

bool HyperscanWrapper::Match(const string& input, set<unsigned int>* results) {
    if (is_compile_required_) {
        last_error_ = "Pattern set was changed but not compiled.";
        return false;
    }
    if (patterns_.empty()) {
        return true;
    }

    results->clear();
    match_results_.clear();
    ch_error_t error = ch_scan(pattern_database_, input.data(), input.size(), 0,
            scratch_, ScanMatchEventHandler, ScanErrorEventHandler, this);
    if (error == CH_SCAN_TERMINATED) {
        // Returns CH_SCAN_TERMINATED if the match callback indicated that scanning should stop.
        last_error_ = "Due to PCRE limitations, the match was stopped: " + to_string(error);
        return false;
    } else if (error != CH_SUCCESS) {
        last_error_ = "An unexpected error in chimera occurred: error = " + to_string(error) + " input = " + input;
        return false;
    }

    results->insert(match_results_.begin(), match_results_.end());
    return true;
}

std::string HyperscanWrapper::GetLastError() const {
    return last_error_;
}

void HyperscanWrapper::CleanUp() {
    ch_error_t error;
    if (scratch_ != nullptr) {
        error = ch_free_scratch(scratch_);
        if (error != CH_SUCCESS) {
            fprintf(stderr, "Unable to free scratch: error = %d\n", error);
        }
        scratch_ = nullptr;
    }
    if (pattern_database_ != nullptr) {
        error = ch_free_database(pattern_database_);
        if (error != CH_SUCCESS) {
            fprintf(stderr, "Unable to free pattern database: error = %d\n", error);
        }
        pattern_database_ = nullptr;
    }
}

void HyperscanWrapper::MatchHandler(unsigned int id, unsigned long long from, unsigned long long to,
        unsigned int flags) {
    match_results_.insert(id);
}

ch_callback_t HyperscanWrapper::ScanMatchEventHandler(unsigned int id, unsigned long long from, unsigned long long to,
        unsigned int flags, unsigned int size, const ch_capture_t *captured, void *context) {
    HyperscanWrapper* wrapper = reinterpret_cast<HyperscanWrapper*>(context);
    wrapper->MatchHandler(id, from, to, flags);
    return CH_CALLBACK_CONTINUE;
}

ch_callback_t HyperscanWrapper::ScanErrorEventHandler(ch_error_event_t error_type, unsigned int id,
        void *info, void *context) {
    // The callback can return CH_CALLBACK_TERMINATE to stop matching.
    // Otherwise, a return value of CH_CALLBACK_CONTINUE will continue, with the current pattern if
    // configured to produce multiple matches per pattern, while a return value of CH_CALLBACK_SKIP_PATTERN
    // will cease matching this pattern but continue matching the next pattern.
    return CH_CALLBACK_TERMINATE;
}

}
