cmake_minimum_required(VERSION 3.1.0)
project("OMXLib")

SET(LIBRARY_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Library output path")
SET(EXECUTABLE_OUTPUT_PATH ${CMAKE_BINARY_DIR}/Bin/${CMAKE_BUILD_TYPE} CACHE PATH "Executable output path")
  
if(MSVC)
	FIND_PACKAGE(HDF5 COMPONENTS C HL NO_MODULE REQUIRED shared)
	add_definitions(-D_SBCS) 
	
	set (LINK_LIBS ${LINK_LIBS} hdf5-shared;hdf5_hl-shared;)
else()
	FIND_PACKAGE(HDF5 COMPONENTS C HL NO_MODULE REQUIRED static)

	set(LINK_LIBS ${LINK_LIBS} ${HDF5_C_STATIC_LIBRARY} ${HDF5_HL_STATIC_LIBRARY})
endif()
# FIND_PACKAGE (HDF5) # Find non-cmake built HDF5


if(CMAKE_COMPILER_IS_GNUCXX)
    #SET(WARNINGS_HELD_FOR_CLEANUP "-pedantic -Wno-unused-variable -Wno-unused-but-set-variable -Wno-reorder")
    SET(WARNINGS_HELD_FOR_CLEANUP "-pedantic -Wno-reorder")
	
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} -std=c++14 -O2 -fPIC -Wall")
	SET(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${WARNINGS_HELD_FOR_CLEANUP}")
endif()


message("CXX_FLAGS: " ${CMAKE_CXX_FLAGS})
add_subdirectory(lib)
add_subdirectory(omxbench)

