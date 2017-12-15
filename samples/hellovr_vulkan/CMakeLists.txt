set(TARGET_NAME hellovr_vulkan)

#add_executable(${TARGET_NAME}
#  ${SHARED_SRC_FILES}
#  hellovr_vulkan.cpp
#)

add_executable(${TARGET_NAME}
  ${SHARED_SRC_FILES}
  buffer.cc
  vrsystem.cc
  vulkansystem.cc
  windowsystem.cc
  global.cc
  refactor_hellovr_vulkan.cpp
)

target_link_libraries(${TARGET_NAME}
  ${SDL2_LIBRARIES}
  ${VULKAN_LIBRARY}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${EXTRA_LIBS}
)

setTargetOutputDirectory(${TARGET_NAME})
