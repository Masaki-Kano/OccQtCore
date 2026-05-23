# cmake/QtDeploy.cmake

function(target_deploy_qt target_name)
    if(WIN32)
        add_custom_command(TARGET ${target_name} POST_BUILD
            COMMAND ${CMAKE_COMMAND} -E echo "Deploying Qt DLLs..."
            COMMAND Qt6::windeployqt
                    --no-translations
                    "$<TARGET_FILE:${target_name}>"
            COMMAND ${CMAKE_COMMAND} -E echo "Qt deploy finished."
            VERBATIM
        )
    endif()
endfunction()
