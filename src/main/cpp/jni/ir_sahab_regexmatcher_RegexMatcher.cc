#include "ir_sahab_regexmatcher_RegexMatcher.h"
#include "hyperscan_wrapper.h"
#include <set>
#include <map>
#include <memory>

using sahab::HyperscanWrapper;

const char* java_assertion_error_path = "java/lang/AssertionError";
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
unsigned int num_created_instances = 0;
std::map<unsigned int, std::unique_ptr<HyperscanWrapper> > instances;

static HyperscanWrapper* GetHyperscanInstance(JNIEnv* jenv, jlong handle) {
    HyperscanWrapper* instance = nullptr;
    try {
        instance = instances.at(static_cast<unsigned int>(handle)).get();
    } catch (std::out_of_range& e) {
        jclass clazz = jenv->FindClass(java_assertion_error_path);
        std::string msg = "Invalid instance handle: handle = ";
        msg += handle;
        if (jenv->ThrowNew(clazz, msg.c_str()) < 0) {
            fprintf(stderr, "Failed to throw exception: %s.", msg.c_str());
            std::exit(EXIT_FAILURE);
        }
    }
    return instance;
}

JNIEXPORT jlong JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_newInstance(
        JNIEnv* jenv, jobject jobj) {
    ++num_created_instances;
    instances[num_created_instances] = std::unique_ptr<HyperscanWrapper>(new HyperscanWrapper());
    return num_created_instances;
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_close(
        JNIEnv* jenv, jobject jobj, jlong jinstance_handle) {
    instances.erase(static_cast<unsigned int>(jinstance_handle));
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_addPattern(
        JNIEnv* jenv, jobject jobj, jlong jinstance_handle, jlong jpattern_id, jstring jpattern,
        jboolean is_case_sensitive) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_handle);
    if (instance == nullptr)
        return;

    auto pattern = jenv->GetStringUTFChars(jpattern, nullptr);
    instance->AddPattern(static_cast<unsigned int>(jpattern_id), pattern, is_case_sensitive);
    jenv->ReleaseStringUTFChars(jpattern, pattern);
}

JNIEXPORT jboolean JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_removePattern(
        JNIEnv* jenv, jobject jobj, jlong jinstance_handle, jlong jpattern_id) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_handle);
    if (instance == nullptr)
        return JNI_FALSE;

    return instance->RemovePattern(static_cast<unsigned int>(jpattern_id)) ?
            JNI_TRUE : JNI_FALSE;
}

JNIEXPORT void JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_preparePatterns(
        JNIEnv* jenv, jobject jobj, jlong jinstance_handle) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_handle);
    if (instance == nullptr)
        return;

    auto result = static_cast<jlong>(instance->CompilePatterns());
    if (result != 0) {
        jclass clazz = jenv->FindClass(java_pattern_preparation_exception_path);
        jmethodID clazz_constructor = jenv->GetMethodID(clazz, "<init>", "(Ljava/lang/String;J)V");
        std::string msg = "Failed to prepare patterns:  ";
        msg.append(instance->GetLastError());
        auto jexception = jenv->NewObject(clazz, clazz_constructor,
                                          jenv->NewStringUTF(msg.c_str()), result);
        if (jenv->Throw(static_cast<jthrowable>(jexception)) < 0) {
            fprintf(stderr, "Failed to throw exception: %s.", msg.c_str());
            std::exit(EXIT_FAILURE);
        }
        return;
    }
}

JNIEXPORT jobject JNICALL Java_ir_sahab_regexmatcher_RegexMatcher_match(
        JNIEnv* jenv, jobject, jlong jinstance_handle, jstring jinput) {
    HyperscanWrapper* instance = GetHyperscanInstance(jenv, jinstance_handle);
    if (instance == nullptr)
        return nullptr;

    auto input = jenv->GetStringUTFChars(jinput, nullptr);
    std::set<unsigned int> results;
    auto error_occurred = !instance->Match(input, &results);
    jenv->ReleaseStringUTFChars(jinput, input);

    if (error_occurred) {
        jclass clazz = jenv->FindClass(java_assertion_error_path);
        if (jenv->ThrowNew(clazz, instance->GetLastError()) < 0) {
            fprintf(stderr, "Failed to throw exception: %s.", instance->GetLastError());
            std::exit(EXIT_FAILURE);
        }
        return nullptr;
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
                fprintf(stderr, "Element was not added to array: %d", result);
            }
        }
    }

    return jresult;
}
