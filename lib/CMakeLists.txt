
add_library(OMXLib SHARED
	include/OmxPlatform.hpp
	include/OmxException.hpp
	include/OmxCommon.hpp
	include/OmxFile.hpp
	include/OmxMatrix.hpp
	include/OmxAttributeCollection.hpp
	include/OmxZonalReference.hpp
	src/OmxAttributeOwnerData.hpp
	src/OmxFileOwnerData.hpp
	src/OmxCommon.cpp
	src/OmxFile.cpp
	src/OmxMatrix.cpp
	src/OmxAttributeCollection.cpp
	src/H5Scoped.hpp
	src/OmxH5Common.hpp
	src/OmxH5Common.cpp
	src/OmxZonalReference.cpp
	)


target_link_libraries (OMXLib ${LINK_LIBS})

target_include_directories(OMXLib PUBLIC $(CMAKE_CURRENT_SOURCE_DIR}/include))
target_include_directories(OMXLib PRIVATE $(CMAKE_CURRENT_SOURCE_DIR}/src))
target_include_directories(OMXLib PRIVATE ${HDF5_INCLUDE_DIR})


