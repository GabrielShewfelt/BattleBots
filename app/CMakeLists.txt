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

# TARGET 1: Main Game
add_executable(BattleBots ${GAME_SOURCES})
target_link_libraries(BattleBots LINK_PRIVATE hal)

# Copy BattleBots to public NFS directory
add_custom_command(TARGET BattleBots POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:BattleBots>"
    "~/cmpt351/public/myApps/BattleBots"
    COMMENT "Copying BattleBots to NFS directory"
)

# TARGET 2: Network Test 
add_executable(test_network ${NET_TEST_SOURCES})
target_link_libraries(test_network LINK_PRIVATE hal)

# Copy test_network to public NFS directory
add_custom_command(TARGET test_network POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:test_network>"
    "~/cmpt351/public/myApps/test_network"
    COMMENT "Copying test_network to NFS directory"
)

# TARGET 3: Controller Test
add_executable(test_controller ${CTRL_TEST_SOURCES})
target_link_libraries(test_controller LINK_PRIVATE hal)

# Copy test_controller to public NFS directory
add_custom_command(TARGET test_controller POST_BUILD
    COMMAND "${CMAKE_COMMAND}" -E copy
    "$<TARGET_FILE:test_controller>"
    "~/cmpt351/public/myApps/test_controller"
    COMMENT "Copying test_controller to NFS directory"
)