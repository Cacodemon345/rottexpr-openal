# rottexpr-openal

This fork of [rottexpr](https://github.com/LTCHIPS/rottexpr) adds OpenAL support.

## Features

1. All the original features of rottexpr.
2. Ability to play back ogg/flac sound files via libsndfile.

## Current limitations/bugs

1. Only MIDI music playback is implemented.
2. Sound playback may be rough around the edges.
3. Windows support isn't tested.
4. Sound reverb isn't implemented (affects Shroom Mode).

## Building

Build using CMake. libsndfile, SDL2 and OpenAL Soft development files must be installed.
