set(ASSETS_GROUP_NAME "Assets")

function(target_assets target)
    cmake_parse_arguments(
        TA
        "NOSTRIP"
        "OUTPUT_NAME;NAMESPACE;DECLARATIONS_VAR"
        "ASSET_FILES;STRIP"
        ${ARGN}
    )

    if(NOT TA_OUTPUT_NAME)
        set(TA_OUTPUT_NAME "${target}-assets")
    endif()

    set(output_file "${TA_OUTPUT_NAME}.cpp")

    if(NOT TA_STRIP AND NOT TA_NOSTRIP)
        set(TA_STRIP "${CMAKE_CURRENT_SOURCE_DIR}")
    endif()

    set(cmd_args -q -o "${output_file}")

    if(TA_STRIP)
        foreach(_S ${TA_STRIP})
            list(APPEND cmd_args -s "${_S}")
        endforeach()
    endif()

    if(TA_NAMESPACE)
        list(APPEND cmd_args -n ${TA_NAMESPACE})
    endif()

    set(raw_asset_files ${TA_ASSET_FILES})
    set(TA_ASSET_FILES "")
    set(DECLARATIONS "")
    foreach(_asset_file ${raw_asset_files})
        if(NOT IS_ABSOLUTE "${_asset_file}")
            set(_asset_file "${CMAKE_CURRENT_SOURCE_DIR}/${_asset_file}")
        endif()
        list(APPEND TA_ASSET_FILES "${_asset_file}")

        set(rel_asset_file ${_asset_file})
        if(TA_STRIP)
            foreach(_S ${TA_STRIP})
                string(REGEX REPLACE "^${_S}" "" rel_asset_file ${rel_asset_file})
            endforeach()
        endif()
        string(REGEX REPLACE "[^A-Za-z0-9_]" "_" sanitized_rel_asset_file "${rel_asset_file}")
        string(REGEX REPLACE "^_" "" sanitized_rel_asset_file ${sanitized_rel_asset_file})
        set(DECLARATIONS "${DECLARATIONS}::ngn::BufferView ${sanitized_rel_asset_file}()\;\n")
    endforeach()
    list(APPEND cmd_args "--")
    list(APPEND cmd_args ${TA_ASSET_FILES})

    add_custom_command(
        OUTPUT "${output_file}"
        COMMAND assetc ${cmd_args}
        DEPENDS assetc ${TA_ASSET_FILES}
        COMMENT "Compiling asset bundle ${TA_OUTPUT}"
        VERBATIM
    )

    target_sources(${target} PRIVATE "${output_file}" ${raw_asset_files})
    source_group(${ASSETS_GROUP_NAME} FILES ${raw_asset_files})

    if(TA_DECLARATIONS_VAR)
        set(${TA_DECLARATIONS_VAR} ${DECLARATIONS} PARENT_SCOPE)
    endif()
endfunction()
