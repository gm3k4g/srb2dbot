install(
    TARGETS srb2dbot_exe
    RUNTIME COMPONENT srb2dbot_Runtime
)

if(PROJECT_IS_TOP_LEVEL)
  include(CPack)
endif()
