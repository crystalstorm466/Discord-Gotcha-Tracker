 cmake_minimum_required(VERSION 3.22)
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Create the executable
add_executable(${PROJECT_NAME}
    src/main.cpp
)

# Find DPP and xlnt (Excel read/write)
find_package(DPP REQUIRED)
find_package(OpenSSL REQUIRED)
find_package(OpenXLSX REQUIRED)
# Include directories
target_include_directories(${PROJECT_NAME} PRIVATE
    ${DPP_INCLUDE_DIRS}
    ${XLSXWRITER_INCLUDE_DIRS}  # if you're still using libxlsxwriter too
)

# Link all required libraries
target_link_libraries(${PROJECT_NAME} PRIVATE
    ${DPP_LIBRARIES}
    OpenXLSX::OpenXLSX 
    OpenSSL::SSL 
    OpenSSL::Crypto
)

# Set C++ version
set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

