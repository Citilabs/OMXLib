
add_executable(omxbench 
	src/omxbench.cpp)
	
include_directories(${PROJECT_SOURCE_DIR}/lib/include)
   
target_link_libraries(omxbench OMXLib)

install (TARGETS omxbench
         RUNTIME DESTINATION ${PROJECT_BINARY_DIR}/bin)


