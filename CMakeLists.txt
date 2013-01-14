cmake_minimum_required( VERSION 2.8 )

# for config.h:
include_directories( "/home/jakob/src/marsyas-0.4.8/build" )

set( plugin_src
    plugin/entry.cpp
    plugin/plugin.cpp
    plugin/marsyas_plugin.cpp
    modules/classification.cpp
    marsystems/buffer.cpp
    marsystems/vampsink.cpp
    marsystems/chromaticentropy.cpp
)

add_library( plugin MODULE ${plugin_src} )

target_link_libraries( plugin vamp-sdk marsyas samplerate fftw3f m )

set_target_properties( plugin PROPERTIES
    OUTPUT_NAME segmentervampplugin
    PREFIX ""
)

install( TARGETS plugin
    DESTINATION "lib/vamp"
)

### Test App

set( test_src
    app/main.cpp
    marsystems/buffer.cpp
    marsystems/vampsink.cpp
)

add_executable( test ${test_src} )

target_link_libraries( test marsyas vamp-sdk )
