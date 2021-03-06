######################################################################
### Tritium Build Script (CMake)                                   ###
### http://gabe.is-a-geek.org/composite/                           ###
######################################################################
CMAKE_MINIMUM_REQUIRED(VERSION 2.4)

if(COMMAND cmake_policy)
  # Libraries linked via full path no longer produce linker search
  # paths.
  cmake_policy(SET CMP0003 NEW)
endif(COMMAND cmake_policy)


######################################################################
### TEST SOURCES AND BUILD                                         ###
######################################################################

IF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
  FILE(GLOB_RECURSE test_HEADERS *.h ../Tritium/*.h ../src/*.h)

  CONFIGURE_FILE(test_config.hpp.in ${CMAKE_CURRENT_BINARY_DIR}/test_config.hpp @ONLY)
  CONFIGURE_FILE(t_Serialization-default.h2song.in ${CMAKE_CURRENT_BINARY_DIR}/t_Serialization-default.h2song)
  INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})

  LIST(APPEND test_LIST
    t_TestTemplate
    t_SeqEvent
    t_SeqScript
    t_SeqScriptPrivate
    t_AudioPort
    t_Mixer
    t_Presets
    t_TritiumXml
    t_Serialization
    t_SimpleTransportMaster
    t_Song
    t_TransportPosition
    t_Sample
    t_ObjectBundle
    t_MidiImplementationBase
    t_DefaultMidiImplementation
    )

  ADD_DEFINITIONS("-DBOOST_TEST_DYN_LINK")
  ADD_DEFINITIONS("-DLADSPA_SUPPORT")
  ADD_DEFINITIONS("-DJACK_SUPPORT")

  ENABLE_TESTING()

  ADD_LIBRARY(test_utils_Tritium
    test_utils.cpp
    )

  FOREACH(T ${test_LIST})
    ADD_EXECUTABLE(${T}
      ${T}.cpp
      ${CMAKE_CURRENT_BINARY_DIR}/test_config.hpp
      ${test_INCLUDES}
      ${tritium_INCLUDES}
      )
    SET_SOURCE_FILES_PROPERTIES(${T}
      PROPERTIES
      COMPILE_FLAGS "-DBOOST_TEST_MAIN"
      )
    # Applications that use Tritium should /not/ have to
    # link to QtCore.  However, in order to scaffold some
    # of the tests, the link is necc.  (E.g. in t_AudioPort
    # where we include a private header).
    TARGET_LINK_LIBRARIES(${T}
      Tritium
      test_utils_Tritium
      ${Boost_UNIT_TEST_FRAMEWORK_LIBRARIES}
      ${QT_LIBRARIES}
      )

    ADD_TEST(${T} ${T})
  ENDFOREACH(T ${test_LIST})

  ######################################################################
  ### CONFIGURATION SUMMARY                                          ###
  ######################################################################

  MESSAGE("\n"
  "Configuration Summary for Tritium Tests\n"
  "---------------------------------------\n"
  )

  MACRO(lib_report name)
    IF(${name}_FOUND)
      message("ENABLED..... ${name}")
    ELSE(${name}_FOUND)
      message("disabled.... ${name}")
    ENDIF(${name}_FOUND)
  ENDMACRO(lib_report)

  lib_report(Boost)
  lib_report(Boost_UNIT_TEST_FRAMEWORK)

ENDIF(Boost_UNIT_TEST_FRAMEWORK_FOUND)
