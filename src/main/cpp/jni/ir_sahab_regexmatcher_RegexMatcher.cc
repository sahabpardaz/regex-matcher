#include "ir_sahab_regexmatcher_RegexMatcher.h"
#include "hyperscan_wrapper.h"
#include <set>
#include <map>
#include <memory>
#include <mutex>
#include <string>
#include <climits>

using sahab::HyperscanWrapper;

const char* java_assertion_error_path = "java/lang/AssertionError";
const char* java_illegal_argument_exception_path = "java/lang/IllegalArgumentException";
const char* java_pattern_preparation_exception_path = "ir/sahab/regexmatcher/exception/PatternPreparationException";

// The whole point of these class is to provide a mapping from Java world to C++ world.
// In fact it does just that using Java native interface (JNI).
//
// Note: This file implements generated functions in ir_sahab_regexmatcher_RegexMatcher.h.
//       The header file can be generated automatically by Java itself. To do this, just run the following
//       commands in order:
//       1) mvn clean compile -pl path/to/regex-matcher -am
//       2) cd path/to/regex-matcher/target/classes
//       3) javah -cp . <full_package_name_of_desired_class> (example ir.sahab.regexmatcher.RegexMatcher)
// Note: We also keep track of live HyperscanWrapper instances. This is done by keeping a map of
//       created instances. An instance is created by calling newInstance() and it is destroyed by calling close().
int64_t num_created_instances = 0;
std::map<int64_t, std::unique_ptr<HyperscanWrapper> > instances;
std::mutex instances_map_mutex;

static void ThrowJavaException(JNIEnv* jenv, const char* java_error_class_path, std::string message) {
    jclass clazz = jenv->FindClass(java_error_class_path);
    if (jenv->ThrowNew(clazz, message.c_str()) < 0) {
        fprintf(stderr, "Failed to throw exception: %s.", message.c_str());
        std::exit(EXIT_FAILURE);
    }
}

static HyperscanWrapper* GetHyperscanInstance(JNIEnv* jenv, jlong jinstance_id) {
    HyperscanWrapper* instance = nullptr;
    auto instance_id = static_cast<int64_t>(jinstance_id);
    try {
        instances_map_mutex.lock();
        instance = instances.at(instance_id).get();
    } catch (std::out_of_range& e) {
        ThrowJavaException(jenv, java_assertion_error_path, "Either instance closed or not valid: Instance ID = "
            + std::to_string(instance_id));
    }
    instances_map_mutex.unlock();
    return instance;
}

JNIEXPORT jlong JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_newInstance(
        JNIEnv* jenv, jobject jobj) {
    instances_map_mutex.lock();
    num_created_instances++;
    instances[num_created_instances] = std::unique_ptr<HyperscanWrapper>(new HyperscanWrapper());
    instances_map_mutex.unlock();
    return static_cast<jlong>(num_created_instances);
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_close(
        JNIEnv* jenv, jobject jobj, jlong jinstance_id) {
    instances_map_mutex.lock();
    instances.erase(static_cast<int64_t>(jinstance_id));
    instances_map_mutex.unlock();
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_addPattern(
        JNIEnv* jenv, jobject jobj, jlong jinstance_id, jlong jpattern_id, jstring jpattern,
        jboolean is_case_sensitive) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_id);
    if (instance == nullptr) {
        return; // C++ continues to work after Java exception is thrown
    }

    auto pattern_id = static_cast<int64_t>(jpattern_id);
    if (pattern_id <= 0 || pattern_id > UINT_MAX) {
        ThrowJavaException(jenv, java_illegal_argument_exception_path, "Pattern ID must between 0 and "
            + std::to_string(UINT_MAX) + ": pattern ID = " + std::to_string(pattern_id));
        return; // C++ continues to work after Java exception is thrown
    }

    auto pattern = jenv->GetStringUTFChars(jpattern, nullptr);
    if (pattern == nullptr) {
        ThrowJavaException(jenv, java_assertion_error_path, "Unable to convert java 'pattern' string to cpp string!");
        return; // C++ continues to work after Java exception is thrown
    }
    instance->AddPattern(static_cast<unsigned int>(pattern_id), pattern, (bool)(is_case_sensitive == JNI_TRUE));
    jenv->ReleaseStringUTFChars(jpattern, pattern);
}

JNIEXPORT jboolean JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_removePattern(
        JNIEnv* jenv, jobject jobj, jlong jinstance_id, jlong jpattern_id) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_id);
    if (instance == nullptr) {
        return JNI_FALSE; // C++ continues to work after Java exception is thrown
    }

    auto pattern_id = static_cast<int64_t>(jpattern_id);
    if (pattern_id <= 0 || pattern_id > UINT_MAX) {
        ThrowJavaException(jenv, java_illegal_argument_exception_path, "Pattern ID must between 0 and "
            + std::to_string(UINT_MAX) + ": pattern ID = " + std::to_string(pattern_id));
        return JNI_FALSE; // C++ continues to work after Java exception is thrown
    }

    return instance->RemovePattern(static_cast<unsigned int>(pattern_id)) ? JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_preparePatterns(
        JNIEnv* jenv, jobject jobj, jlong jinstance_id) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_id);
    if (instance == nullptr) {
        return; // C++ continues to work after Java exception is thrown
    }

    auto result = instance->CompilePatterns();
    if (result != 0) {
        jclass clazz = jenv->FindClass(java_pattern_preparation_exception_path);
        jmethodID clazz_constructor = jenv->GetMethodID(clazz, "<init>", "(Ljava/lang/String;J)V");
        std::string msg = "Failed to prepare patterns: " + instance->GetLastError();
        auto jexception = jenv->NewObject(clazz, clazz_constructor,
                                          jenv->NewStringUTF(msg.c_str()), static_cast<jlong>(result));
        if (jenv->Throw(static_cast<jthrowable>(jexception)) < 0) {
            fprintf(stderr, "Failed to throw exception: %s.", msg.c_str());
            std::exit(EXIT_FAILURE);
        }
    }
    return;
}

JNIEXPORT jobject JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_match(
        JNIEnv* jenv, jobject, jlong jinstance_id, jstring jinput) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_id);
    if (instance == nullptr) {
        return nullptr; // C++ continues to work after Java exception is thrown
    }

    auto input = jenv->GetStringUTFChars(jinput, nullptr);
    if (input == nullptr) {
        ThrowJavaException(jenv, java_assertion_error_path, "Unable to convert java 'input' string to cpp string!");
        return nullptr; // C++ continues to work after Java exception is thrown
    }
    std::set<unsigned int> results;
    auto error_occurred = !instance->Match(input, &results);
    jenv->ReleaseStringUTFChars(jinput, input);

    if (error_occurred) {
        ThrowJavaException(jenv, java_assertion_error_path, instance->GetLastError());
        return nullptr; // C++ continues to work after Java exception is thrown
    }

    auto jarraylist_clazz = jenv->FindClass("java/util/ArrayList");
    auto jarraylist_constructor = jenv->GetMethodID(jarraylist_clazz, "<init>", "()V");
    auto jresult = jenv->NewObject(jarraylist_clazz, jarraylist_constructor);

    if (!results.empty()) {
        auto jarraylist_add = jenv->GetMethodID(jarraylist_clazz, "add", "(Ljava/lang/Object;)Z");
        auto jlong_clazz = jenv->FindClass("java/lang/Long");
        auto jlong_constructor = jenv->GetMethodID(jlong_clazz, "<init>", "(J)V");
        for (auto& result : results) {
            auto element = jenv->NewObject(jlong_clazz, jlong_constructor, static_cast<jlong>(result));
            if (JNI_TRUE != jenv->CallBooleanMethod(jresult, jarraylist_add, element)) {
                ThrowJavaException(jenv, java_assertion_error_path, "Element was not added to array: "
                    + std::to_string(result));
                return nullptr; // C++ continues to work after Java exception is thrown
            }
        }
    }

    return jresult;
}