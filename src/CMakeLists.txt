add_subdirectory(audiolib_v2)

find_package(ZMusic REQUIRED)

add_executable(rottexpr cin_actr.c
cin_efct.c
cin_evnt.c
cin_glob.c
cin_main.c
cin_util.c
dosutil.c
engine.c
isr.c
modexlib.c
rt_actor.c
rt_battl.c
rt_build.c
rt_cfg.c
rt_crc.c
rt_com.c
rt_debug.c
rt_dmand.c
rt_door.c
rt_draw.c
rt_floor.c
rt_game.c
rt_in.c
rt_main.c
rt_map.c
rt_menu.c
rt_msg.c
rt_net.c
rt_playr.c
rt_rand.c
rt_scale.c
rt_sound.c
rt_sqrt.c
rt_stat.c
rt_state.c
rt_str.c
rt_ted.c
rt_util.c
rt_view.c
rt_vid.c
rt_err.c
scriplib.c
w_wad.c
watcom.c
z_zone.c
byteordr.c
dukemusc.cpp
winrott.c
queue.c
HashTable.c)

target_compile_definitions(rottexpr PRIVATE -DSHAREWARE=0 -DSITELICENSE=0 -DSUPERROTT=0 -DUSE_SDL=1 -DPLATFORM_UNIX=1)
find_package(SDL2 REQUIRED)
target_link_libraries(rottexpr PRIVATE SDL2::SDL2 audiolib ${ZMUSIC_LIBRARIES})