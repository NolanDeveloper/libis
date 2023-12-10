add_library(libis
        libis.c
        libis_buffer_source.c
        libis_file_source.c
        libis_internal.h
        libis_source.h)
if(LINUX)
    target_sources(libis
            libis_file_descriptor_source.c)
endif(LINUX)

target_link_libraries(libis
        PUBLIC libis_interface)
