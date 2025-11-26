# 1. Include local headers
include_directories(include)

# 2. SEPARATE SOURCES
# We cannot compile main.c and test_network.c into the same binary.

# Group A: The Main Game
# Manually listing files is safer, or we use GLOB and filter.
# Here we list the specific files needed for the game manager:
set(GAME_SOURCES 
    src/main.c 
    src/bot.c 
    src/bot_controller.c 
    src/network.c
    src/oled.c
)

# Group B: The Network Test
set(TEST_SOURCES 
    src/test_network.c 
    src/network.c
)

# --- TARGET 1: Main Game Application ---
add_executable(BattleBots ${GAME_SOURCES})

# Link HAL (Hardware Abstraction Layer) and Pthreads
target_link_libraries(BattleBots LINK_PRIVATE hal)

# Copy executable to final location
add_custom_command(TARGET BattleBots POST_BUILD 
  COMMAND "${CMAKE_COMMAND}" -E copy 
      "$<TARGET_FILE:BattleBots>"
      "~/cmpt351/public/myApps/BattleBots" 
  COMMENT "Copying BattleBots executable to public NFS directory"
)

# --- TARGET 2: Network Test Script ---
# This creates a separate executable called "test_network"
add_executable(test_network ${TEST_SOURCES})

# Even the test might need HAL headers if network.c relies on them (optional but safe)
target_link_libraries(test_network LINK_PRIVATE hal)
