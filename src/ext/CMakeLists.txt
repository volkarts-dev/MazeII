add_library(ext STATIC
    StbImage.cpp StbImage.hpp
    StbVorbis.hpp StbVorbis.cpp
)

target_include_directories(ext PUBLIC .)
