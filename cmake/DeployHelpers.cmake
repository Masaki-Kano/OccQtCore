# cmake/DeployHelpers.cmake

function(target_deploy_occt target_name)
    add_custom_command(TARGET ${target_name} POST_BUILD
        COMMAND ${CMAKE_COMMAND} -E echo "Deploying OCCT DLLs..."
        COMMAND "${CMAKE_SOURCE_DIR}/tools/deploy_occt_dlls.bat"
                "${OCC_BIN_DIR}"
                "${OCC_3RDPARTY_DIR}"
                "$<TARGET_FILE_DIR:${target_name}>"
        VERBATIM
    )
endfunction()
