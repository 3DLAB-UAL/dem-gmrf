# See docs online: https://github.com/3DLAB-UAL/dem-gmrf
PROJECT(demgrmf)

# These commands are needed by modern versions of CMake:
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)
if(COMMAND cmake_policy)
    cmake_policy(SET CMP0003 NEW)  # Required by CMake 2.7+
	if(POLICY CMP0043)
		cmake_policy(SET CMP0043 OLD) #  Ignore COMPILE_DEFINITIONS_<Config> properties.
	endif()
endif()

SET(EXECUTABLE_OUTPUT_PATH "${CMAKE_BINARY_DIRECTORY}/bin")

#  See: http://www.mrpt.org/Libraries
FIND_PACKAGE(MRPT REQUIRED maps;gui)

# ---------------------------------------------
# TARGET:
# ---------------------------------------------
# Define the executable target:
ADD_EXECUTABLE(dem-gmrf src/dem-gmrf_main.cpp  ) 
TARGET_LINK_LIBRARIES(dem-gmrf 
	${MRPT_LIBS}  # This is filled by FIND_PACKAGE(MRPT ...)
	)

# Set optimized building:
IF(CMAKE_COMPILER_IS_GNUCXX AND NOT CMAKE_BUILD_TYPE MATCHES "Debug")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -O3")
ENDIF()

