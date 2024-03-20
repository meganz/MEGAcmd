function(header_from_source)
    set(SOURCE ${ARGV1})
    string(REPLACE ".cpp" ".h" header_ ${SOURCE})
    if(EXISTS ${header_})
        set(${ARGV0} ${header_} PARENT_SCOPE)
    endif()
endfunction()

function(add_source_and_corresponding_header_to_target)
    set(options "")
    set(oneValueArgs "")
    set(multiValueArgs FLAG INTERFACE PUBLIC PRIVATE)

    set(TARGET ${ARGV0})

    cmake_parse_arguments(PARSE_ARGV 1 "target_sources_conditional"
        "${options}"
        "${oneValueArgs}"
        "${multiValueArgs}"
    )
    foreach(_source ${target_sources_conditional_PUBLIC})
        header_from_source(header ${_source})
        target_sources(${TARGET} PUBLIC ${_source} ${header})
    endforeach()

    foreach(_source ${target_sources_conditional_PRIVATE})
        header_from_source(header ${_source})
        target_sources(${TARGET} PRIVATE ${_source} ${header})
    endforeach()
endfunction()


