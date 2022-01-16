set(WG_RESOURCES_DIR ${CMAKE_CURRENT_SOURCE_DIR})
set(WG_RESOURCES_TARGET_DIR ${CMAKE_RUNTIME_OUTPUT_DIRECTORY}/resources)
set(WG_RESOURCES_FILES
    img/statue.png)

if (NOT MSVC)
    add_custom_command(
        OUTPUT ${WG_RESOURCES_TARGET_DIR}
        COMMAND ${CMAKE_COMMAND} -E make_directory ${WG_RESOURCES_TARGET_DIR}
        COMMENT "Creating directory ${WG_RESOURCES_TARGET_DIR}")
    list(APPEND WG_RESOURCES_OUTPUTS ${WG_RESOURCES_TARGET_DIR})
endif ()

foreach (FileRelativePath ${WG_RESOURCES_FILES})
    if (MSVC)
        set(FILE_DEPENDENCIES_LIST ${WG_RESOURCES_DIR}/${FileRelativePath})
    else ()
        set(FILE_DEPENDENCIES_LIST ${WG_RESOURCES_DIR}/${FileRelativePath} ${WG_RESOURCES_TARGET_DIR})
    endif ()

    add_custom_command(
        OUTPUT ${WG_RESOURCES_TARGET_DIR}/${FileRelativePath}
        COMMAND ${CMAKE_COMMAND} -E copy_if_different
        ${WG_RESOURCES_DIR}/${FileRelativePath} ${WG_RESOURCES_TARGET_DIR}/${FileRelativePath}
        DEPENDS ${FILE_DEPENDENCIES_LIST}
        COMMENT "Copying ${WG_RESOURCES_TARGET_DIR}/${FileRelativePath}")

    list(APPEND WG_RESOURCES_OUTPUTS ${WG_RESOURCES_TARGET_DIR}/${FileRelativePath})
    list(APPEND WG_RESOURCES_INPUTS ${WG_RESOURCES_DIR}/${FileRelativePath})
endforeach ()

add_custom_target(wengine-resources
    DEPENDS ${WG_RESOURCES_OUTPUTS} ${WG_RESOURCES_INPUTS}
    SOURCES ${WG_RESOURCES_INPUTS})
