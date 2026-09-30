#!/usr/bin/env python3
"""Exchange raw saves with an independently built FCEUmm libretro core.

Run from tests/, after make, with the core path as the only argument.
Generated ROM/save fixtures stay in .build/sram-interop/. No commercial ROM.
The original MMC1 homebrew program loads a 16-byte song record from SRAM
into CPU RAM so both emulators must actually execute the loaded data.
"""
import ctypes as C
import hashlib
import pathlib
import subprocess
import sys

core = C.CDLL(str(pathlib.Path(sys.argv[1]).resolve()))
out = pathlib.Path('.build/sram-interop').resolve()
out.mkdir(parents=True, exist_ok=True)
rom = bytearray(16 + 32768 + 8192)
rom[:16] = bytes([78, 69, 83, 26, 2, 1, 0x12, 0, 1, 0, 0, 0, 0, 0, 0, 0])
# SEI; CLD; LDX #0; LDA $6000,X; STA $10,X; INX; CPX #16;
# BNE $8004; JMP $8002. Repeatedly load the synthetic song's sixteen notes.
program = bytes([0x78, 0xD8, 0xA2, 0, 0xBD, 0, 0x60, 0x95, 0x10,
                 0xE8, 0xE0, 16, 0xD0, 0xF6, 0x4C, 2, 0x80])
rom[16:16 + len(program)] = program
rom[16 + 0x7FFA:16 + 0x8000] = bytes([0, 0x80] * 3)
rom_path = out / 'song-loader.nes'
rom_path.write_bytes(rom)

class Game(C.Structure):
    _fields_ = [('path', C.c_char_p), ('data', C.c_void_p),
                ('size', C.c_size_t), ('meta', C.c_char_p)]

class Info(C.Structure):
    _fields_ = [('name', C.c_char_p), ('version', C.c_char_p),
                ('extensions', C.c_char_p), ('fullpath', C.c_bool),
                ('block_extract', C.c_bool)]

@C.CFUNCTYPE(C.c_bool, C.c_uint, C.c_void_p)
def environment(command, data):
    return command in (10, 11, 16, 18)  # Pixel format and static descriptors.

@C.CFUNCTYPE(None, C.c_void_p, C.c_uint, C.c_uint, C.c_size_t)
def video(*args):
    pass

@C.CFUNCTYPE(C.c_size_t, C.c_void_p, C.c_size_t)
def audio(data, frames):
    return frames

@C.CFUNCTYPE(None)
def poll():
    pass

@C.CFUNCTYPE(C.c_int16, C.c_uint, C.c_uint, C.c_uint, C.c_uint)
def input_state(*args):
    return 0

core.retro_set_environment(environment)
core.retro_set_video_refresh(video)
core.retro_set_audio_sample_batch(audio)
core.retro_set_input_poll(poll)
core.retro_set_input_state(input_state)
core.retro_get_memory_data.argtypes = [C.c_uint]
core.retro_get_memory_data.restype = C.c_void_p
core.retro_get_memory_size.argtypes = [C.c_uint]
core.retro_get_memory_size.restype = C.c_size_t
core.retro_load_game.argtypes = [C.POINTER(Game)]
core.retro_load_game.restype = C.c_bool
info = Info()
core.retro_get_system_info(C.byref(info))
core.retro_init()
rom_buffer = C.create_string_buffer(bytes(rom))
game = Game(str(rom_path).encode(), C.cast(rom_buffer, C.c_void_p), len(rom), None)
assert core.retro_load_game(C.byref(game))
assert core.retro_get_memory_size(0) == 8192  # RETRO_MEMORY_SAVE_RAM
sram = core.retro_get_memory_data(0)
song = bytes([48, 52, 55, 60, 55, 52, 48, 43, 45, 48, 52, 57, 52, 48, 45, 40])
payload = song + bytes((i * 29 + i // 256) & 255 for i in range(16, 8192))
C.memmove(sram, payload, len(payload))
for _ in range(3):
    core.retro_run()
assert C.string_at(core.retro_get_memory_data(2) + 0x10, 16) == song
external = out / 'fceumm.sav'
external.write_bytes(C.string_at(sram, 8192))
exported = out / 'racknes.sav'
subprocess.run(['.build/racknes', '--sram-interop', str(rom_path),
                str(external), str(exported)], check=True)
core.retro_unload_game()
assert core.retro_load_game(C.byref(game))
sram = core.retro_get_memory_data(0)
C.memset(sram, 0, 8192)
C.memmove(sram, exported.read_bytes(), 8192)
for _ in range(3):
    core.retro_run()
assert C.string_at(core.retro_get_memory_data(2) + 0x10, 16) == song
assert C.string_at(sram, 8192) == payload == external.read_bytes() == exported.read_bytes()
core.retro_unload_game()
core.retro_deinit()
print('External emulator:', info.name.decode(), info.version.decode())
print('ROM SHA-256:', hashlib.sha256(rom).hexdigest())
print('Save SHA-256:', hashlib.sha256(payload).hexdigest(), 'size:', len(payload))
print('Both emulators loaded song notes:', list(song))
