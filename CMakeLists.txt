cmake_minimum_required( VERSION 2.8 )

# for config.h:
include_directories( "/home/jakob/src/marsyas-0.4.8/build" )

set( plugin_src
    plugin/entry.cpp
    plugin/plugin.cpp
    plugin/marsyas_plugin.cpp
)

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

add_library( plugin MODULE ${plugin_src} ${modules_src} ${marsystems_src} )

target_link_libraries( plugin vamp-sdk marsyas samplerate fftw3f m )

set_target_properties( plugin PROPERTIES
    OUTPUT_NAME segmentervampplugin
    PREFIX ""
)

install( TARGETS plugin
    DESTINATION "lib/vamp"
)

### Test App

set( app_src
    app/app.cpp
)

add_executable( segmenter ${app_src} ${modules_src} )

target_link_libraries( segmenter sndfile samplerate fftw3f vamp-sdk m )

