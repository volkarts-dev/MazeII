function(enable_avx2 target)
    if(MSVC)
        set(options /arch:AVX2)
    else()
        set(options -mavx2)
    endif()

    target_compile_options(${target} INTERFACE ${options})
endfunction()
