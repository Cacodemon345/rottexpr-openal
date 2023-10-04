find_package( OpenAL REQUIRED )
find_package( SndFile REQUIRED )
find_package( Threads REQUIRED )

add_library(audiolib STATIC fx_man.cpp)

find_library(MATH_LIBRARY m)

target_link_libraries(audiolib PRIVATE OpenAL::OpenAL SndFile::sndfile Threads::Threads)

if (MATH_LIBRARY)
    target_link_libraries(audiolib PRIVATE m)
endif()