cmake_minimum_required (VERSION 3.10)

# No project definition here, the project is defined at top level file.

# Check JDK 8 installed
find_package(Java 1.8 COMPONENTS Development)
include_directories(${JNI_INCLUDE_DIRS})

add_library(chimera_jni SHARED
            ir_sahab_regexmatcher_RegexMatcher.cc
            hyperscan_wrapper.cc)
target_link_libraries(chimera_jni ${LIBCHIMERA_LIBRARIES} ${LIBHYPERSCAN_LIBRARIES} ${LIBPCRE} ${JNI_LIBRARIES})
