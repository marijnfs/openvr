set(TARGET_NAME hellovr_vulkan)

#add_executable(${TARGET_NAME}
#  ${SHARED_SRC_FILES}
#  hellovr_vulkan.cpp
#)

link_directories(/usr/local/cuda/lib64)
find_package(CUDA QUIET REQUIRED)
find_package(OpenCV REQUIRED)


cuda_add_executable(${TARGET_NAME}
  ${SHARED_SRC_FILES}
  buffer.cc
  vrsystem.cc
  vulkansystem.cc
  windowsystem.cc
  global.cc
  script.cc
  flywheel.cc
  scene.cc
  framerenderbuffer.cc
  refactor_hellovr_vulkan.cpp
  learningsystem.cc
  )
cuda_add_cublas_to_target(${TARGET_NAME})


target_link_libraries(${TARGET_NAME}
  ${SDL2_LIBRARIES}
  ${VULKAN_LIBRARY}
  ${OPENVR_LIBRARIES}
  ${CMAKE_DL_LIBS}
  ${EXTRA_LIBS}
  ${CUDA_curand_LIBRARY}
  ${OpenCV_LIBS}
  -llua5.3
  -lcapnp
  -lmcdnn
  -lvlstm
  -lcuda
  -lcudnn
  -lz
  )

target_include_directories(${TARGET_NAME} PRIVATE /home/marijnfs/dev/vlstm/inc)
target_include_directories(${TARGET_NAME} PRIVATE /home/marijnfs/dev/mcdnn/inc)
target_include_directories(${TARGET_NAME} PRIVATE /usr/local/cuda/include)



setTargetOutputDirectory(${TARGET_NAME})
