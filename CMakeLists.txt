cmake_minimum_required(VERSION 3.16)

# Set RainMaker path - UPDATE THIS TO YOUR PATH
if(DEFINED ENV{RMAKER_PATH})
    set(RMAKER_PATH $ENV{RMAKER_PATH})
else()
    # Default paths - change according to your system
    if(WIN32)
        set(RMAKER_PATH "C:/esp/esp-rainmaker")
    else()
        set(RMAKER_PATH "$ENV{HOME}/esp/esp-rainmaker")
    endif()
endif()

message(STATUS "RainMaker path: ${RMAKER_PATH}")

# Add RainMaker and project components
set(EXTRA_COMPONENT_DIRS 
    #${RMAKER_PATH}/components/esp-insights/components 
    ${RMAKER_PATH}/components
    ${RMAKER_PATH}/examples/common
    ${CMAKE_CURRENT_LIST_DIR}/components
)

# Project version for OTA
set(PROJECT_VER "1.0.0")

# Set partition table
set(PARTITION_CSV_PATH ${CMAKE_CURRENT_LIST_DIR}/partitions.csv)

include($ENV{IDF_PATH}/tools/cmake/project.cmake)
project(smart_environmental_logger)
