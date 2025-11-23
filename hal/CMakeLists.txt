# Build a library (`hal`) which exposes the header files as "hal/*.h"

# 1. Gather Sources
file(GLOB MY_SOURCES "src/*.c")

# 2. Create the Static Library
add_library(hal STATIC ${MY_SOURCES})

# 3. Expose the include directory to any target that links this library
# This allows app/src files to do: #include "hal/controller.h"
target_include_directories(hal PUBLIC include)