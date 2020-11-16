# only for cmake --version >= 3.5.1
cmake_minimum_required(VERSION 3.5.1)

# project name
project(bmp2vox)
set(EXEC bmp2vox)
set(CMAKE_CXX_STANDARD 14)
include_directories(easybmp)
file(GLOB SOURCES ./*.cpp ./*.h ./easybmp/*.h ./easybmp/*.cpp)

set(Boost_USE_STATIC_LIBS OFF) 
set(Boost_USE_MULTITHREADED ON)  
set(Boost_USE_STATIC_RUNTIME OFF) 
find_package(Boost 1.56.0 COMPONENTS filesystem program_options REQUIRED) 

if(Boost_FOUND)
    include_directories(${Boost_INCLUDE_DIRS}) 
    # compiles the files defined by SOURCES to generate the executable defined by EXEC
    add_executable(${EXEC} ${SOURCES})
    target_link_libraries(${EXEC} ${Boost_LIBRARIES})
endif()
