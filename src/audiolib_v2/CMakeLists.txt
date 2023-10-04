find_package( OpenAL REQUIRED )

add_library(audiolib STATIC fx_man.cpp)

find_library(MATH_LIBRARY m)

target_link_libraries(audiolib PRIVATE OpenAL::OpenAL)

if (MATH_LIBRARY)
    target_link_libraries(audiolib PRIVATE m)
endif()