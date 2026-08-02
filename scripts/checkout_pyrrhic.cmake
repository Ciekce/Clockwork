find_package(Git)

if (NOT IS_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../vendor/Pyrrhic")
    message(STATUS "Cloning Pyrrhic...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} clone https://github.com/official-clockwork/Pyrrhic
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../vendor")
endif()

file(READ "${CMAKE_CURRENT_LIST_DIR}/../vendor/pyrrhic_commit.txt" PYRRHIC_COMMIT)
string(STRIP ${PYRRHIC_COMMIT} PYRRHIC_COMMIT)

message(STATUS "Checking out Pyrrhic commit ${PYRRHIC_COMMIT}...")

execute_process(
    COMMAND ${GIT_EXECUTABLE} show ${PYRRHIC_COMMIT}
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../vendor/Pyrrhic"
    RESULT_VARIABLE COMMIT_NOT_PRESENT
    OUTPUT_QUIET
    ERROR_QUIET)

if (COMMIT_NOT_PRESENT)
    message(STATUS "Fetching from Pyrrhic fork...")
    execute_process(
        COMMAND ${GIT_EXECUTABLE} fetch
        WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../vendor/Pyrrhic"
        RESULT_VARIABLE FETCH_FAILED)

    if (FETCH_FAILED)
        message(FATAL_ERROR "Could not fetch Pyrrhic from fork.")
    endif()
endif ()

execute_process(
    COMMAND ${GIT_EXECUTABLE} -c advice.detachedHead=false checkout ${PYRRHIC_COMMIT}
    WORKING_DIRECTORY "${CMAKE_CURRENT_LIST_DIR}/../vendor/Pyrrhic"
    RESULT_VARIABLE CHECKOUT_FAILED)

if (CHECKOUT_FAILED)
    message(FATAL_ERROR "Could not checkout Pyrrhic commit ${PYRRHIC_COMMIT}")
endif()
