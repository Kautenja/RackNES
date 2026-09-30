#!/usr/bin/env python3
"""Capture unfiltered NES color indices from a user-built Nestopia libretro core.

Usage: reference_nestopia.py CORE ROM PREFIX INPUT_SCRIPT [FRAME_COUNT]
No core, game ROM, or game assets are downloaded by this helper.
"""
import ctypes as c
from pathlib import Path
import sys

if len(sys.argv) not in (5, 6):
    raise SystemExit(__doc__)
core, rom, prefix, script = sys.argv[1:5]
limit = int(sys.argv[5]) if len(sys.argv) == 6 else 1500
assert 0 < limit <= 36000
lib = c.CDLL(str(Path(core).resolve()))
ENV = c.CFUNCTYPE(c.c_bool, c.c_uint, c.c_void_p)
VIDEO = c.CFUNCTYPE(None, c.c_void_p, c.c_uint, c.c_uint, c.c_size_t)
SAMPLE = c.CFUNCTYPE(None, c.c_int16, c.c_int16)
BATCH = c.CFUNCTYPE(c.c_size_t, c.c_void_p, c.c_size_t)
POLL = c.CFUNCTYPE(None)
STATE = c.CFUNCTYPE(c.c_int16, c.c_uint, c.c_uint, c.c_uint, c.c_uint)


class Variable(c.Structure):
    _fields_ = [("key", c.c_char_p), ("value", c.c_char_p)]


class GameInfo(c.Structure):
    _fields_ = [("path", c.c_char_p), ("data", c.c_void_p),
                ("size", c.c_size_t), ("meta", c.c_char_p)]


class SystemInfo(c.Structure):
    _fields_ = [("name", c.c_char_p), ("version", c.c_char_p),
                ("extensions", c.c_char_p), ("fullpath", c.c_bool),
                ("block_extract", c.c_bool)]


# A caller-owned working directory supplies the optional Nestopia database.
root = str(Path.cwd()).encode()
options = {
    b"nestopia_palette": b"raw",
    b"nestopia_force_system": b"ntsc",
    b"nestopia_blargg_ntsc_filter": b"disabled",
}
for edge in (b"v_top", b"v_bottom", b"h_left", b"h_right"):
    options[b"nestopia_overscan_" + edge] = b"0"


@ENV
def environment(command, data):
    if command in (9, 31):  # System/save directory.
        c.cast(data, c.POINTER(c.c_char_p))[0] = root
        return True
    if command == 10:  # XRGB8888.
        return c.cast(data, c.POINTER(c.c_int))[0] == 1
    if command == 15:  # Core variable lookup.
        variable = c.cast(data, c.POINTER(Variable)).contents
        if variable.key in options:
            variable.value = options[variable.key]
            return True
    if command == 16:  # Accept legacy core option definitions.
        return True
    if command == 17:
        c.cast(data, c.POINTER(c.c_bool))[0] = False
        return True
    return False


frame = 0
buttons = [0, 0]
capture_errors = []


@VIDEO
def video(data, width, height, pitch):
    if not data or (frame % 60 and frame != limit):
        return
    if width != 256 or height != 240:
        capture_errors.append(f"Unexpected frame size: {width} x {height}")
        return
    indices = bytearray()
    for y in range(height):
        row = (c.c_uint32 * width).from_address(data + y * pitch)
        # Nestopia's raw palette encodes the color nibble in R, level in G,
        # emphasis in B. RackNES currently has no emphasis support.
        indices.extend(((p >> 16) & 255) // 17 +
                       16 * (((p >> 8) & 255) // 85) for p in row)
    try:
        Path(f"{prefix}-{frame}.pgm").write_bytes(b"P5\n256 240\n63\n" + indices)
    except OSError as error:
        capture_errors.append(str(error))


@SAMPLE
def sample(left, right):
    pass


@BATCH
def batch(data, count):
    return count


@POLL
def poll():
    pass


@STATE
def state(port, device, index, button):
    # Libretro A/B differ from the NES A/B bit ordering.
    mapping = {8: 0, 0: 1, 2: 2, 3: 3, 4: 4, 5: 5, 6: 6, 7: 7}
    return bool(port < 2 and button in mapping and
                buttons[port] & (1 << mapping[button]))


lib.retro_set_environment(environment)
lib.retro_set_video_refresh(video)
lib.retro_set_audio_sample(sample)
lib.retro_set_audio_sample_batch(batch)
lib.retro_set_input_poll(poll)
lib.retro_set_input_state(state)
lib.retro_init()
system = SystemInfo()
lib.retro_get_system_info(c.byref(system))
print(system.name.decode(), system.version.decode())
payload = Path(rom).read_bytes()
buffer = c.create_string_buffer(payload)
info = GameInfo(str(Path(rom).resolve()).encode(), c.cast(buffer, c.c_void_p),
                len(payload), None)
lib.retro_load_game.argtypes = [c.POINTER(GameInfo)]
lib.retro_load_game.restype = c.c_bool
assert lib.retro_load_game(c.byref(info))
inputs = {}
previous = -1
for line in Path(script).read_text().splitlines():
    boundary, p1, p2 = map(int, line.split())
    assert boundary > previous and 0 <= p1 <= 255 and 0 <= p2 <= 255
    inputs[boundary] = [p1, p2]
    previous = boundary
for port in (0, 1):
    lib.retro_set_controller_port_device(port, 1)
for frame in range(1, limit + 1):
    if frame - 1 in inputs:
        buttons = inputs[frame - 1]
    lib.retro_run()
    if capture_errors:
        raise RuntimeError(capture_errors[0])
lib.retro_unload_game()
lib.retro_deinit()
print(f"Captured {limit} NTSC frames without cropping or filtering")
