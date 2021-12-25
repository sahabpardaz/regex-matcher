add_definitions(
    -std=c++11
    -D_GLIBCXX_PERMIT_BACKWARD_HASH
    -DNDEBUG
    -Wall
    -Wno-sign-compare
    -Wno-unused-local-typedefs
    -fno-builtin-malloc
    -fno-builtin-calloc
    -fno-builtin-realloc
    -fno-builtin-free
)

# Enable optimization
add_definitions(
    -O2
)

# We want to link libgcc and libstdc++ statically, so that debugging will be easier
SET(CMAKE_EXE_LINKER_FLAGS "-static-libgcc -static-libstdc++")
