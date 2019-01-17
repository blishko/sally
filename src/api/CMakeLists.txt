add_library(api SHARED sally_api.cpp)
target_sources(api PUBLIC ${CMAKE_SOURCE_DIR}/src/parse_options.cpp)
target_include_directories(api PRIVATE ${CMAKE_SOURCE_DIR})

set_target_properties(api PROPERTIES PUBLIC_HEADER "sally_api.h")
set_target_properties(api PROPERTIES OUTPUT_NAME "sally")

if (SALLY_STATIC_BUILD)
    set_target_properties(api PROPERTIES LINK_FLAGS "-static")
    set_target_properties (api PROPERTIES LINK_SEARCH_START_STATIC ON)
    set_target_properties (api PROPERTIES LINK_SEARCH_END_STATIC ON)
endif()

# Link in all the directories
#foreach(DIR ${sally_SRC_DIRS})
#    #message(${DIR})
#    link_directories(${sally_BINARY_DIR}/src/${DIR})
#    set(sally_LIBS ${DIR} ${sally_LIBS})
#endforeach(DIR)

# Link in all the other libraries
target_link_libraries(api  ${sally_LIBS})
if (YICES2_FOUND)
    target_link_libraries(api  ${YICES2_LIBRARY})
endif()
if (LIBPOLY_FOUND)
    target_link_libraries(api  ${LIBPOLY_LIBRARY})
endif()
if (MATHSAT5_FOUND)
    target_link_libraries(api  ${MATHSAT5_LIBRARY})
endif()
if (Z3_FOUND)
    target_link_libraries(api  ${Z3_LIBRARY})
endif()
if (OPENSMT2_FOUND)
    set_property(TARGET api PROPERTY CXX_STANDARD 11)
    set_property(TARGET smt PROPERTY CXX_STANDARD 11)
    set_property(TARGET api PROPERTY CXX_STANDARD_REQUIRED ON)
    set_property(TARGET smt PROPERTY CXX_STANDARD_REQUIRED ON)
    target_link_libraries(api  ${OPENSMT2_LIB})
    target_compile_definitions(smt PUBLIC -DPRODUCE_PROOF)
endif()

target_link_libraries(api ${Boost_LIBRARIES} ${GMP_LIBRARY})
target_link_libraries(api libantlr3c)

install(TARGETS api
        LIBRARY
        DESTINATION lib
        COMPONENT Library
        PUBLIC_HEADER
        DESTINATION include
        COMPONENT Library
        )