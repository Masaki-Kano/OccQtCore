cmake_minimum_required(VERSION 3.19)

project(OccQtCore LANGUAGES CXX)

set(CMAKE_CXX_STANDARD 17)
set(CMAKE_CXX_STANDARD_REQUIRED ON)

find_package(Qt6 6.5 REQUIRED COMPONENTS
    Core
    Widgets
)

qt_standard_project_setup()

include(cmake/OccConfig.cmake)
include(cmake/DeployHelpers.cmake)
include(cmake/QtDeploy.cmake)

option(DEPLOY_OCCT_ON_BUILD "Deploy OCCT runtime DLLs after build" OFF)
option(DEPLOY_QT_ON_BUILD "Deploy Qt runtime DLLs after build" OFF)


qt_add_executable(OccQtCore
    WIN32
    MACOSX_BUNDLE

    app/main.cpp
    app/MainWindow.cpp
    app/MainWindow.h
    app/MainWindow.ui

    cmake/OccConfig.cmake
    cmake/DeployHelpers.cmake
    cmake/QtDeploy.cmake
    tools/deploy_occt_dlls.bat

    src/Log/LogMessage.h
    src/Log/AppLogger.h
    src/Log/AppLogger.cpp
    src/Log/LogPanel.h
    src/Log/LogPanel.cpp

    src/View/OccView.h
    src/View/OccView.cpp

    src/IO/StepLoader.h
    src/IO/StepLoader.cpp

    src/Core/DocumentData.h
    src/Core/DocumentData.cpp


    src/Geometry/GeometryTypes.h
    src/Core/PickResult.h
    src/Core/SelectionInfo.h
    src/Geometry/GeometryModel.h
    src/Geometry/GeometryModel.cpp
    src/Geometry/GeometryAnalyzer.h
    src/Geometry/GeometryAnalyzer.cpp
    src/Geometry/GeometryGraph.h
    src/Geometry/GeometryGraph.cpp
    src/Log/AppLogReporter.h
    src/Log/AppLogReporter.cpp
    src/Feature/FeatureTypes.h
    src/Feature/HoleFeatureRecognizer.h
    src/Feature/HoleFeatureRecognizer.cpp
    src/Geometry/TopologyQuery.h
    src/Core/CollectionUtil.h
    src/Geometry/TopologyQuery.cpp
    src/Geometry/SurfaceUtil.h
    src/Geometry/SurfaceUtil.cpp

    src/View/DisplayTypes.h
    src/View/DisplayStyle.h
    src/View/DisplayStyle.cpp
    src/Log/LogFormatUtil.h
    src/Log/LogFormatUtil.cpp
    src/Log/HoleRecognitionLogReporter.h
    src/Log/HoleRocognitionLogReporter.cpp
    src/Log/GeometryLogReporter.h
    src/Log/GeometryLogReporter.cpp
    src/Feature/HoleRecognitionModel.h


    src/Debug/HoleDebugPanel.h src/Debug/HoleDebugPanel.cpp src/Debug/HoleDebugPanel.ui




    src/Feature/HoleContextGeometryGrouper.h
    src/Feature/HoleContextGeometryGrouper.cpp
    src/Feature/HoleContextTracePortBuilder.h
    src/Feature/HoleContextTracePortBuilder.cpp
    src/Feature/HoleContextTraceStepBuilder.h
    src/Feature/HoleContextTraceStepBuilder.cpp
    src/Feature/HoleContextPlanarGroupBuilder.h
    src/Feature/HoleContextPlanarGroupBuilder.cpp
    src/Feature/HoleContextCylindricalGroupBuilder.h
    src/Feature/HoleContextCylinderGroupBuilder.cpp
    src/Feature/HoleContextGeometryGroupRegistry.h
    src/Feature/HoleContextGeometryGroupRegistry.cpp
    src/Feature/HoleContextTraceExplorer.h
    src/Feature/HoleContextTraceExplorer.cpp
    src/Feature/HoleContextTraversalPolicy.h
    src/Feature/HoleContextTraversalPolicy.cpp
    src/Feature/HoleContextConnectionPolicy.h
    src/Feature/HoleContextConnectionPolicy.cpp
    src/Feature/HoleContextSeedWallFinder.h
    src/Feature/HoleContextSeedWallFinder.cpp
    src/Feature/HoleRecognitionWorkingData.h

)

target_include_directories(OccQtCore
    PRIVATE
    ${CMAKE_CURRENT_SOURCE_DIR}/src
    ${CMAKE_CURRENT_SOURCE_DIR}/app
)

target_link_libraries(OccQtCore
    PRIVATE
        Qt6::Core
        Qt6::Widgets
)

target_link_occt(OccQtCore)

if(DEPLOY_OCCT_ON_BUILD)
    target_deploy_occt(OccQtCore)
endif()

if(DEPLOY_QT_ON_BUILD)
    target_deploy_qt(OccQtCore)
endif()

