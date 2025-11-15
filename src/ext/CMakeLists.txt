add_library(ext STATIC
    StbImage.cpp StbImage.hpp
)

target_include_directories(ext PUBLIC .)
