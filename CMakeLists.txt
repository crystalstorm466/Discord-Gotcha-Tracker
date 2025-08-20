cmake_minimum_required(VERSION 3.22)
project(discord-bot VERSION 1.0 DESCRIPTION "A discord bot")

list(APPEND CMAKE_MODULE_PATH ${CMAKE_CURRENT_SOURCE_DIR}/cmake)

# Use find_package for DPP, as vcpkg is configured to handle this.
# This is the correct way when using vcpkg's toolchain file.
find_package(dpp CONFIG REQUIRED) # Note: typically 'dpp' lowercase for vcpkg

# Find other packages
find_package(OpenSSL REQUIRED)
find_package(OpenXLSX REQUIRED)

# Add your executables
add_executable(${PROJECT_NAME} src/main.cpp)
add_executable(getmembers src/getmembers.cpp) # Your getmembers executable

# Configure the main project executable
target_include_directories(${PROJECT_NAME} PRIVATE
    # dpp::dpp provides its own include directories through its target properties
    # No need for manual DPP_INCLUDE_DIR if using find_package(dpp CONFIG REQUIRED)
    ${XLSXWRITER_INCLUDE_DIRS} # This might be part of OpenXLSX::OpenXLSX's interface properties.
                               # Check OpenXLSX docs, you might not need this line if it's modern CMake.
)

target_link_libraries(${PROJECT_NAME} PRIVATE
    dpp::dpp # Link to the imported target provided by find_package(dpp CONFIG REQUIRED)
    OpenXLSX::OpenXLSX
    OpenSSL::SSL
    OpenSSL::Crypto
)

set_target_properties(${PROJECT_NAME} PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
)

# Configure the getmembers executable separately
target_include_directories(getmembers PRIVATE
    # Similar to above, dpp::dpp provides its own include paths
    ${XLSXWRITER_INCLUDE_DIRS} # If getmembers also uses OpenXLSX
)

target_link_libraries(getmembers PRIVATE
    dpp::dpp # Link DPP to getmembers
    OpenXLSX::OpenXLSX # If getmembers uses OpenXLSX
    OpenSSL::SSL # If getmembers uses OpenSSL
    OpenSSL::Crypto # If getmembers uses OpenSSL
)

set_target_properties(getmembers PROPERTIES
    CXX_STANDARD 20
    CXX_STANDARD_REQUIRED ON
) 
