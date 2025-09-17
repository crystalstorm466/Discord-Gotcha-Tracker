CMAKE_FLAGS=-DCMAKE_TOOLCHAIN_FILE=~/vcpkg/scripts/buildsystems/vcpkg.cmake -DCMAKE_EXPORT_COMPILE_COMMANDS=ON
BUILD_DIR=build


all:
	cmake -B $(BUILD_DIR) -S . $(CMAKE_FLAGS)
	cmake --build $(BUILD_DIR)

getmembers:
	cmake -B ${BUILD_DIR} -S . $(CMAKE_FLAGS)
	cmake --build ${BUILD_DIR} --target getmembers

clean:
	rm -rf $(BUILD_DIR)
