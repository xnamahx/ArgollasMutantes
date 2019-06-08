cmake_minimum_required(VERSION 3.0)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-api/script/max-pretarget.cmake)

include_directories( 
	"${C74_INCLUDES}"
)

include_directories(
        "${CMAKE_CURRENT_SOURCE_DIR}/mi"
)

set(RINGS_SRC
       mi/rings/resources.cc
       mi/rings/dsp/string.cc
       mi/rings/dsp/fm_voice.cc
       mi/rings/dsp/resonator.cc
       mi/rings/dsp/part.cc
       mi/rings/dsp/string_synth_part.cc
       )

set(MI_COMMON_SRC
       mi/stmlib/dsp/units.cc
       mi/stmlib/dsp/atan.cc 
       mi/stmlib/utils/random.cc
)

set(MILIB_SRC 
        ${MI_COMMON_SRC}
        ${RINGS_SRC}
        )

add_library(MILib ${MILIB_SRC} )

add_library( 
	${PROJECT_NAME} 
	MODULE
	${PROJECT_NAME}.cpp
)

target_link_libraries(${PROJECT_NAME} MILib)

include(${CMAKE_CURRENT_SOURCE_DIR}/../../max-api/script/max-posttarget.cmake)
