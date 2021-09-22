# Acts compiler flags
if (NOT CMAKE_BUILD_TYPE)
  set(CMAKE_BUILD_TYPE RelWithDebInfo CACHE STRING "Build type configuration" FORCE)
  message(STATUS "Setting default build type: ${CMAKE_BUILD_TYPE}")
endif() 

set(ACTS_CXX_FLAGS "-Wall -Wextra -Wpedantic -Wshadow -Wunused-local-typedefs")
#set(ACTS_CXX_FLAGS_DEBUG "--coverage")
set(ACTS_CXX_FLAGS_MINSIZEREL "")
set(ACTS_CXX_FLAGS_RELEASE "")
set(ACTS_CXX_FLAGS_RELWITHDEBINFO "")

# Acts linker flags
set(ACTS_EXE_LINKER_FLAGS_DEBUG "--coverage")
set(ACTS_SHARED_LINKER_FLAGS_DEBUG "--coverage ")

# assign to global CXX flags
set(CMAKE_CXX_FLAGS "${CMAKE_CXX_FLAGS} ${ACTS_CXX_FLAGS}")
set(CMAKE_CXX_FLAGS_DEBUG "${CMAKE_CXX_FLAGS_DEBUG} ${ACTS_CXX_FLAGS_DEBUG}")
set(CMAKE_CXX_FLAGS_MINSIZEREL "${CMAKE_CXX_FLAGS_MINSIZEREL} ${ACTS_CXX_FLAGS_MINSIZEREL}")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} ${ACTS_CXX_FLAGS_RELEASE}")
set(CMAKE_CXX_FLAGS_RELWITHDEBINFO "${CMAKE_CXX_FLAGS_RELWITHDEBINFO} ${ACTS_CXX_FLAGS_RELWITHDEBINFO}")

# assign to global linker flags
set(CMAKE_EXE_LINKER_FLAGS_DEBUG "${CMAKE_EXE_LINKER_FLAGS_DEBUG} ${ACTS_EXE_LINKER_FLAGS_DEBUG}")
set(CMAKE_SHARED_LINKER_FLAGS_DEBUG "${CMAKE_SHARED_LINKER_FLAGS_DEBUG} ${ACTS_SHARED_LINKER_FLAGS_DEBUG}")

# silence warning about missing RPATH on Mac OSX
set(CMAKE_MACOSX_RPATH 1)

# bake where we found external dependencies, if they were not in the default library directories
set(CMAKE_INSTALL_RPATH_USE_LINK_PATH TRUE)
# set relative library path for ACTS libraries
set(CMAKE_INSTALL_RPATH "\$ORIGIN/../${CMAKE_INSTALL_LIBDIR}")