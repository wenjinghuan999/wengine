set(WG_DYNAMIC_SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag
)
set(WG_DYNAMIC_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/dynamic)

foreach(File ${WG_DYNAMIC_SHADER_FILES})
    get_filename_component(Filename ${File} NAME)
    add_custom_command(
        OUTPUT ${WG_DYNAMIC_SHADER_DIR}/${Filename}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different ${File} ${WG_DYNAMIC_SHADER_DIR}/${Filename}
        DEPENDS ${File}
        COMMENT "Copying ${Filename}"
    )
    list(APPEND WG_DYNAMIC_SHADER_OUTPUTS ${WG_DYNAMIC_SHADER_DIR}/${Filename})
endforeach()

add_custom_target(wengine-shader-dynamic DEPENDS
    DEPENDS ${WG_DYNAMIC_SHADER_FILES} ${WG_DYNAMIC_SHADER_OUTPUTS}
    SOURCES ${WG_DYNAMIC_SHADER_FILES}
)