cmake_minimum_required( VERSION 2.8 )

set( sources
    app/main.cpp
)

#add_executable( test ${sources} )
#target_link_libraries( test samplerate fftw3f )

add_library( plugin MODULE plugin/plugin.cpp )
target_link_libraries( plugin vamp-sdk samplerate fftw3f m )
set_target_properties( plugin PROPERTIES
    OUTPUT_NAME segmentervampplugin
    PREFIX ""
)

install( TARGETS plugin
    DESTINATION "lib/vamp"
)
