# rottexpr-openal

This fork of [rottexpr](https://github.com/LTCHIPS/rottexpr) adds OpenAL support.

## Features

1. All the original features of rottexpr.
2. Ability to play back ogg/flac sound files via libsndfile.

## Current limitations/bugs

1. Windows support isn't tested.
2. Sound reverb is implemented exactly like in Build engine source port Raze, complete with all its limitations.

## Building

Build using CMake. ZMusic, SDL2 and OpenAL Soft development files must be installed. If you are building this project on Windows you may need to define `ZMUSIC_LIBRARIES` and `ZMUSIC_INCLUDE_DIR` to point to your local ZMusic build's built libraries and include directory respectively.
