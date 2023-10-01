add_library(audiolib STATIC fx_man.c
dsl.c
ll_man.c
multivoc.c
mv_mix.c
mvreverb.c
nodpmi.c
pitch.c
user.c
usrhooks.c)
find_package(SDL2_mixer REQUIRED)
target_link_libraries(audiolib PRIVATE SDL2_mixer::SDL2_mixer)