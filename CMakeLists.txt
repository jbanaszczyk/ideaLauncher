cmake_minimum_required(VERSION 4.0)
project(ideaLauncher)

set(CMAKE_CXX_STANDARD 17)

project(ideaLauncher LANGUAGES CXX)

add_executable(ideaLauncher main.cpp appicon.rc)

# C++17
set_target_properties(ideaLauncher PROPERTIES
        CXX_STANDARD 17
        CXX_STANDARD_REQUIRED YES
)

# wWinMain + brak konsoli
target_link_options(ideaLauncher PRIVATE -municode -mwindows)
target_link_libraries(ideaLauncher PRIVATE bcrypt)

# Deploy directory copy after build
set(SCRIPTS_SOURCE_DIR "${CMAKE_SOURCE_DIR}/files_to_copy")
set(DEPLOY_DIR "${CMAKE_SOURCE_DIR}/deploy")


set(DATA_SOURCE_DIR "${SCRIPTS_SOURCE_DIR}/data")
set(DEPLOY_DATA_DIR "${DEPLOY_DIR}/data")

set(APP_SOURCE_DIR "${SCRIPTS_SOURCE_DIR}/app")
set(DEPLOY_APP_DIR "${DEPLOY_DIR}/app")

add_custom_command(TARGET ideaLauncher POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E make_directory "${DEPLOY_DIR}"
        COMMAND ${CMAKE_COMMAND} -E copy_if_different "$<TARGET_FILE:ideaLauncher>" "${DEPLOY_DIR}/$<TARGET_FILE_NAME:ideaLauncher>"
        # Copy all files and directories from files_to_copy into deploy
        COMMAND ${CMAKE_COMMAND} -E copy_directory "${SCRIPTS_SOURCE_DIR}" "${DEPLOY_DIR}"
)
