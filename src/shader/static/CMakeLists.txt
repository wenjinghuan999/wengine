set(WG_STATIC_SHADER_FILES
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.vert
    ${CMAKE_CURRENT_SOURCE_DIR}/simple.frag
)
set(WG_STATIC_SHADER_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/shader/static)

foreach(File ${WG_STATIC_SHADER_FILES})
    get_filename_component(Filename ${File} NAME)
    add_custom_command(
        OUTPUT ${WG_STATIC_SHADER_DIR}/${Filename}.spv
        COMMAND ${Vulkan_GLSLANG_VALIDATOR_EXECUTABLE} -V ${File} -o ${WG_STATIC_SHADER_DIR}/${Filename}.spv
        DEPENDS Vulkan::glslangValidator ${File}
        COMMENT "Compiling shader ${Filename}"
    )
    list(APPEND WG_STATIC_SHADER_OUTPUTS ${WG_STATIC_SHADER_DIR}/${Filename}.spv)
endforeach()

add_custom_target(wengine-shader-static
    DEPENDS ${WG_STATIC_SHADER_FILES} ${WG_STATIC_SHADER_OUTPUTS}
    SOURCES ${WG_STATIC_SHADER_FILES}
)