# cmake/OccConfig.cmake

set(OCC_DIST_ROOT "C:/dev/occt_vc14-64-combined")
set(OCC_ROOT      "${OCC_DIST_ROOT}/occt_vc14-64")

set(OCC_INCLUDE_DIR "${OCC_ROOT}/inc")
set(OCC_LIB_DIR     "${OCC_ROOT}/win64/vc14/lib")
set(OCC_BIN_DIR     "${OCC_ROOT}/win64/vc14/bin")

set(OCC_3RDPARTY_DIR "${OCC_DIST_ROOT}/3rdparty-vc14-64")

set(OCC_LIBS
    TKernel
    TKMath
    TKG2d
    TKG3d
    TKGeomBase
    TKGeomAlgo
    TKBRep
    TKTopAlgo
    TKPrim
    TKBO
    TKBool
    TKShHealing
    TKXSBase
    TKDESTEP
    TKV3d
    TKOpenGl
    TKService
)

function(target_link_occt target_name)
    target_include_directories(${target_name}
        PRIVATE
            ${OCC_INCLUDE_DIR}
    )

    target_link_directories(${target_name}
        PRIVATE
            ${OCC_LIB_DIR}
    )

    target_link_libraries(${target_name}
        PRIVATE
            ${OCC_LIBS}
    )
endfunction()
