install(
    TARGETS ${CMAKE_PROJECT_NAME}
    RUNTIME COMPONENT ${CMAKE_PROJECT_NAME}_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
    include(CPack)
endif()
