include_directories(${rjmcmc_INCLUDE_DIRS})
add_definitions(${rjmcmc_DEFINITIONS})
include_directories(../core)
include_directories(${TIFF_INCLUDE_DIR})

if(WIN32)
  set(exe_FLAG "WIN32")
endif()

file( GLOB SRCS *.c *.cpp *.cxx *.h *.hpp *.hxx ../core/*.hpp ../core/*.cpp)
add_executable( building_footprint_rectangle_gui ${exe_FLAG} ${SRCS} )
target_link_libraries( building_footprint_rectangle_gui ${rjmcmc_LIBRARIES})

