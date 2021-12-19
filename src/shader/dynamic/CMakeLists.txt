set(WG_DYNAMIC_SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag
)
set(WG_DYNAMIC_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/dynamic)

add_custom_command(
    OUTPUT ${WG_DYNAMIC_SHADER_DIR}
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WG_DYNAMIC_SHADER_DIR}
)
list(APPEND WG_DYNAMIC_SHADER_OUTPUTS ${WG_DYNAMIC_SHADER_DIR})

foreach(File ${WG_DYNAMIC_SHADER_FILES})
    get_filename_component(Filename ${File} NAME)
    add_custom_command(
        OUTPUT ${WG_DYNAMIC_SHADER_DIR}/${Filename}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${File} ${WG_DYNAMIC_SHADER_DIR}/${Filename}
        DEPENDS ${File} ${WG_DYNAMIC_SHADER_DIR}
        COMMENT "Copying ${Filename}"
    )
    list(APPEND WG_DYNAMIC_SHADER_OUTPUTS ${WG_DYNAMIC_SHADER_DIR}/${Filename})
endforeach()

add_custom_target(wengine-shader-dynamic DEPENDS
    DEPENDS ${WG_DYNAMIC_SHADER_FILES} ${WG_DYNAMIC_SHADER_OUTPUTS}
    SOURCES ${WG_DYNAMIC_SHADER_FILES}
)