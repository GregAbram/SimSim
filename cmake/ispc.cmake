SET(ISPC_VERSION 1.8.1)

IF (NOT EXISTS "${ISPC_DIRECTORY}/ispc")
  MESSAGE("Could not find ISPC (tried ${ISPC_DIRECTORY}.")
  MESSAGE(FATAL_ERROR "Could not find ISPC. Exiting")
ELSE()
  SET(ISPC_BINARY ${ISPC_DIRECTORY}/ispc)
ENDIF()

# ##################################################################
# add macro ADD_DEFINITIIONS_ISCP() that allows to pass #define's to
# ispc sources
# ##################################################################
SET (ISPC_DEFINES "")
MACRO (ADD_DEFINITIONS_ISPC)
  SET (ISPC_DEFINES ${ISPC_DEFINES} ${ARGN})
ENDMACRO ()

# ##################################################################
# add macro INCLUDE_DIRECTORIES_ISPC() that allows to specify search
# paths for ispc sources
# ##################################################################
SET (ISPC_INCLUDE_DIR "")
MACRO (INCLUDE_DIRECTORIES_ISPC)
  SET (ISPC_INCLUDE_DIR ${ISPC_INCLUDE_DIR} ${ARGN})
ENDMACRO ()

MACRO(CONFIGURE_ISPC)
ENDMACRO()

INCLUDE_DIRECTORIES(${CMAKE_CURRENT_BINARY_DIR})
MACRO (ispc_compile)
  IF (CMAKE_OSX_ARCHITECTURES STREQUAL "i386")
    SET(ISPC_ARCHITECTURE "x86")
  ELSE()
    SET(ISPC_ARCHITECTURE "x86-64")
  ENDIF()
  
  IF(ISPC_INCLUDE_DIR)
    STRING(REGEX REPLACE ";" ";-I;" ISPC_INCLUDE_DIR_PARMS "${ISPC_INCLUDE_DIR}")
    SET(ISPC_INCLUDE_DIR_PARMS "-I" ${ISPC_INCLUDE_DIR_PARMS}) 
  ENDIF()

	SET(CMAKE_ISPC_FLAGS --target=${BUILD_ISA} --addressing=32)

	IF (${CMAKE_BUILD_TYPE} STREQUAL "Release")
    SET(CMAKE_ISPC_OPT_FLAGS -O3)
	ELSE()
    SET(CMAKE_ISPC_OPT_FLAGS -O2 -g)
	ENDIF()

  SET(ISPC_OBJECTS "")
  FOREACH(src ${ARGN})
    SET(ISPC_TARGET_DIR ${CMAKE_CURRENT_BINARY_DIR})

    GET_FILENAME_COMPONENT(fname ${src} NAME_WE)
    GET_FILENAME_COMPONENT(dir ${src} PATH)

    IF ("${dir}" MATCHES "^/")
      # ------------------------------------------------------------------
      # global path name to input, like when we include the embree sources
      # from a global external embree checkout
      # ------------------------------------------------------------------
      STRING(REGEX REPLACE "^/" "${CMAKE_CURRENT_BINARY_DIR}/rebased/" outdir "${dir}")
      SET(indir "${dir}")
      SET(input "${indir}/${fname}.ispc")
    ELSE()
      # ------------------------------------------------------------------
      # local path name to input, like local ospray sources
      # ------------------------------------------------------------------
      SET(outdir "${CMAKE_CURRENT_BINARY_DIR}/local_${dir}")
      SET(indir "${CMAKE_CURRENT_SOURCE_DIR}/${dir}")
      SET(input "${indir}/${fname}.ispc")
    ENDIF()
    SET(outdirh ${ISPC_TARGET_DIR})

    SET(deps "")
    IF (EXISTS ${outdir}/${fname}.dev.idep)
      FILE(READ ${outdir}/${fname}.dev.idep contents)
      STRING(REGEX REPLACE " " ";"     contents "${contents}")
      STRING(REGEX REPLACE ";" "\\\\;" contents "${contents}")
      STRING(REGEX REPLACE "\n" ";"    contents "${contents}")
      FOREACH(dep ${contents})
	      IF (EXISTS ${dep})
	        SET(deps ${deps} ${dep})
	      ENDIF (EXISTS ${dep})
      ENDFOREACH(dep ${contents})
    ENDIF ()

		SET(outputs ${outdir}/${fname}.dev.o ${outdirh}/${fname}_ispc.h)
    SET(ISPC_OBJECTS ${ISPC_OBJECTS} ${outdir}/${fname}.dev.o)
    SET(main_output ${outdir}/${fname}.dev.o)

    ADD_CUSTOM_COMMAND(
      OUTPUT ${outputs}
      COMMAND mkdir -p ${outdir} \; ${ISPC_BINARY}
      -I ${CMAKE_CURRENT_SOURCE_DIR} 
      ${ISPC_INCLUDE_DIR_PARMS}
      ${ISPC_DEFINES}
      --arch=${ISPC_ARCHITECTURE}
      --pic
      ${CMAKE_ISPC_OPT_FLAGS}
      --woff
      ${CMAKE_ISPC_FLAGS}
      --opt=fast-math
      -h ${outdirh}/${fname}_ispc.h
      -MMM  ${outdir}/${fname}.dev.idep 
      -o ${main_output}
      ${input}
      \;
      DEPENDS ${input}
      ${deps})
  ENDFOREACH()
  
ENDMACRO()

MACRO (add_ispc_executable name)
  SET(ISPC_SOURCES "")
  SET(OTHER_SOURCES "")
  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(ext ${src} EXT)
    IF (ext STREQUAL ".ispc")
      SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    ELSE ()
      SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    ENDIF ()
  ENDFOREACH()
  ISPC_COMPILE(${ISPC_SOURCES})
  ADD_EXECUTABLE(${name} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()

MACRO (ISPC_ADD_LIBRARY name type)
  SET(ISPC_SOURCES "")
  SET(OTHER_SOURCES "")
  SET(ISPC_OBJECTS "")
  FOREACH(src ${ARGN})
    GET_FILENAME_COMPONENT(ext ${src} EXT)
    IF (ext STREQUAL ".ispc")
      SET(ISPC_SOURCES ${ISPC_SOURCES} ${src})
    ELSE ()
      SET(OTHER_SOURCES ${OTHER_SOURCES} ${src})
    ENDIF ()
  ENDFOREACH()
  ISPC_COMPILE(${ISPC_SOURCES})
  ADD_LIBRARY(${name} ${type} ${ISPC_OBJECTS} ${OTHER_SOURCES})
ENDMACRO()
