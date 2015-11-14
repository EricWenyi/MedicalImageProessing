cmake_minimum_required(VERSION 2.8)
 
project(EdgeRepairFilter)
 
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
find_package( OpenCV REQUIRED )
include_directories( ${OpenCV_INCLUDE_DIRS} )

if (ITKVtkGlue_LOADED)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()
 
add_executable(EdgeRepairFilter MACOSX_BUNDLE EdgeRepairFilter.cxx)
 
if( "${ITK_VERSION_MAJOR}" LESS 4 )
  target_link_libraries(EdgeRepairFilter ITKReview ${ITK_LIBRARIES} ${OpenCV_LIBS})
else( "${ITK_VERSION_MAJOR}" LESS 4 )
  target_link_libraries(EdgeRepairFilter ${ITK_LIBRARIES} ${OpenCV_LIBS})
endif( "${ITK_VERSION_MAJOR}" LESS 4 )