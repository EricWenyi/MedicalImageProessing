cmake_minimum_required(VERSION 2.8)
 
project(GDCMImageMedianFilter3DTo3D)
 
find_package(ITK REQUIRED)
include(${ITK_USE_FILE})
if (ITKVtkGlue_LOADED)
  find_package(VTK REQUIRED)
  include(${VTK_USE_FILE})
endif()
 
add_executable(GDCMImageMedianFilter3DTo3D MACOSX_BUNDLE GDCMImageMedianFilter3DTo3D.cxx)
 
if( "${ITK_VERSION_MAJOR}" LESS 4 )
  target_link_libraries(GDCMImageMedianFilter3DTo3D ITKReview ${ITK_LIBRARIES})
else( "${ITK_VERSION_MAJOR}" LESS 4 )
  target_link_libraries(GDCMImageMedianFilter3DTo3D ${ITK_LIBRARIES})
endif( "${ITK_VERSION_MAJOR}" LESS 4 )