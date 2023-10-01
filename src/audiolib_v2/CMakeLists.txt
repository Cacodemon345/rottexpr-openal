find_package( OpenAL REQUIRED )
find_package( SndFile REQUIRED )

add_library(audiolib STATIC fx_man.c)

find_library(MATH_LIBRARY m)

target_link_libraries(audiolib PRIVATE OpenAL::OpenAL SndFile::sndfile)

if (MATH_LIBRARY)
    target_link_libraries(audiolib PRIVATE m)
endif()