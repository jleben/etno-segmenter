cmake_minimum_required( VERSION 2.8 )

option(BUILD_VAMP_PLUGIN "Build Vamp plugin." ON)
option(BUILD_EXTRACT_APP "Build extract executable." ON)

set(CMAKE_MODULE_PATH ${CMAKE_SOURCE_DIR}/cmake)

set( modules_src
    modules/pipeline.cpp
    modules/classification.cpp
)

set( marsystems_src
    marsystems/buffer.cpp
    marsystems/linear_trend.cpp
    marsystems/mfcc.cpp
    marsystems/chromaticentropy.cpp
    marsystems/etno_muse_classifier.cpp
    marsystems/vampsink.cpp
)

if(BUILD_VAMP_PLUGIN)
    # for config.h:
    include_directories( "/home/jakob/src/marsyas-0.4.8/build" )

    set( plugin_src
        plugin/entry.cpp
        plugin/plugin.cpp
        plugin/marsyas_plugin.cpp
    )

    add_library( plugin MODULE ${plugin_src} ${modules_src} ${marsystems_src} )

    target_link_libraries( plugin vamp-sdk marsyas samplerate fftw3f m )

    set_target_properties( plugin PROPERTIES
        OUTPUT_NAME segmentervampplugin
        PREFIX ""
    )

    if(CMAKE_SYSTEM_NAME MATCHES Linux)
        install( TARGETS plugin DESTINATION "lib/vamp" )
    endif()
endif()

if(BUILD_EXTRACT_APP)
    find_package(Dependencies)

    include_directories(
        ${BOOST_PROGRAM_OPTIONS_INCLUDE_DIR}
        ${FFTW_INCLUDE_DIR}
        ${SAMPLERATE_INCLUDE_DIR}
        ${SNDFILE_INCLUDE_DIR}
        ${VAMP_SDK_INCLUDE_DIR}
    )

    set( app_src
        app/app.cpp
    )

    add_executable( extract ${app_src} ${modules_src} )

    target_link_libraries( extract
        ${BOOST_PROGRAM_OPTIONS_LIBRARY}
        ${SNDFILE_LIBRARY}
        ${SAMPLERATE_LIBRARY}
        ${FFTW_LIBRARY}
        ${VAMP_SDK_LIBRARY}
        m
    )
endif()
