# This is the root ITK CMakeLists file.
cmake_minimum_required(VERSION 2.8.9)
if(COMMAND CMAKE_POLICY)
  cmake_policy(SET CMP0003 NEW)
endif()


# This project is designed to be built outside the Insight source tree.
project(DICOMHeader)

# Find ITK.
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

add_executable( DICOMHeader DICOMHeader.cxx )

target_link_libraries(DICOMHeader ${ITK_LIBRARIES} ${OpenCV_LIBS})
