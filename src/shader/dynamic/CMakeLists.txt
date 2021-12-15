set(WG_DYNAMIC_SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag
)
set(WG_DYNAMIC_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/dynamic)

add_custom_target(wengine-shader-dynamic DEPENDS ${WG_DYNAMIC_SHADER_FILES})
add_custom_command(TARGET wengine-shader-dynamic PRE_BUILD
    COMMAND ${CMAKE_COMMAND} -E make_directory ${WG_DYNAMIC_SHADER_DIR}
)

foreach(File ${WG_DYNAMIC_SHADER_FILES})
    get_filename_component(Filename ${File} NAME)
    add_custom_command(
        TARGET wengine-shader-dynamic POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E
        copy_if_different ${File} ${WG_DYNAMIC_SHADER_DIR}
        DEPENDS ${File}
        COMMENT "Copying ${Filename}"
    )
endforeach()