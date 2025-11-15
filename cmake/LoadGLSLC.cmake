find_program(GLSLC_EXE
    NAMES glslc
    REQUIRED
)
message(STATUS "Found glslc: ${GLSLC_EXE}")

function(compile_shader_files)
    cmake_parse_arguments(
        CSF
        ""
        "VAR"
        "SHADER_FILES"
        ${ARGN}
    )

    foreach(s ${CSF_SHADER_FILES})
        if(NOT IS_ABSOLUTE "${s}")
            set(source "${CMAKE_CURRENT_SOURCE_DIR}/${s}")
        endif()

        set(output "${CMAKE_CURRENT_BINARY_DIR}/${s}.spv")
        set(depfile "${CMAKE_CURRENT_BINARY_DIR}/${s}.d")

        add_custom_command(
            OUTPUT "${output}"
            COMMAND
                "${GLSLC_EXE}"
                -MD -MF "${depfile}"
                -o "${output}"
                "${source}"
            DEPENDS "${source}"
            DEPFILE "${depfile}"
            COMMENT "Compiling GLSLC ${s}.spv"
            WORKING_DIRECTORY "${CMAKE_CURRENT_SOURCE_DIR}"
        )

        if (CSF_VAR)
            list(APPEND ${CSF_VAR} "${output}")
            set(${CSF_VAR} "${${CSF_VAR}}" PARENT_SCOPE)
        endif()
    endforeach()
endfunction()
