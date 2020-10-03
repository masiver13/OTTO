otto_include_board(parts/ui/egl)
otto_include_board(parts/audio/rtaudio)
otto_include_board(parts/controller/otto-mcu-i2c)
#otto_include_board(parts/rpi)
set(CMAKE_LINKER_FLAGS_RELEASE "${CMAKE_LINKER_FLAGS_RELEASE} -ffast-math -funsafe-math-optimizations -mfpu=neon-vfpv4")
set(CMAKE_CXX_FLAGS_RELEASE "${CMAKE_CXX_FLAGS_RELEASE} -ffast-math -funsafe-math-optimizations -mfpu=neon-vfpv4")
target_link_libraries(loguru PUBLIC execinfo)
