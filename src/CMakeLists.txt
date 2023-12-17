add_library(libis
        libis.c
        libis_buffer_source.c
        libis_file_source.c
        libis_internal.h
        libis_source.h
	$<${LINUX}:libis_file_descriptor_source.c>)

target_link_libraries(libis
        PUBLIC libis_interface)

