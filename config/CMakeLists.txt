set(WG_CONFIG_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/config.json)
set(WG_CONFIG_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/config)

if (NOT MSVC)
    add_custom_command(
        OUTPUT ${WG_CONFIG_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${WG_CONFIG_DIR}
        COMMENT "Creating directory ${WG_CONFIG_DIR}")
    list(APPEND WG_CONFIG_OUTPUTS ${WG_CONFIG_DIR})
endif ()

foreach (File ${WG_CONFIG_FILES})
    get_filename_component(Filename ${File} NAME)

    if (MSVC)
        set(CONFIG_DEPENDENCIES_LIST ${File})
    else ()
        set(CONFIG_DEPENDENCIES_LIST ${File} ${WG_CONFIG_DIR})
    endif ()

    add_custom_command(
        OUTPUT ${WG_CONFIG_DIR}/${Filename}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${File} ${WG_CONFIG_DIR}/${Filename}
        DEPENDS ${CONFIG_DEPENDENCIES_LIST}
        COMMENT "Copying ${Filename}")

    list(APPEND WG_CONFIG_OUTPUTS ${WG_CONFIG_DIR}/${Filename})
endforeach ()

add_custom_target(wengine-config
    DEPENDS ${WG_CONFIG_FILES} ${WG_CONFIG_OUTPUTS}
    SOURCES ${WG_CONFIG_FILES})
