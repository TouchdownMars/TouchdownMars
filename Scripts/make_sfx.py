# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Synthesize the game's UI and scene sounds (design.md §5 sound tokens) as 16-bit mono WAV files.

Run with any Python 3 (no packages needed):  python Scripts/make_sfx.py
Output: Data/Audio/SFX_TD_*.wav. Import with Scripts/import_sfx.py inside the editor.
These sounds are made by this script (sine, noise and envelopes): team-made, not AI-generated, Apache 2.0.
"""

import math
import os
import random
import struct
import wave

RATE = 44100
OUT = os.path.join(os.path.dirname(os.path.abspath(__file__)), "..", "Data", "Audio")
rng = random.Random(14112026)


def env(t, attack, release, length):
    if t < attack:
        return t / attack
    if t > length - release:
        return max(0.0, (length - t) / release)
    return 1.0


def tone(freqs, length, volume=0.5, attack=0.005, release=0.05, noise=0.0, glide=0.0):
    samples = []
    for i in range(int(RATE * length)):
        t = i / RATE
        value = 0.0
        for f in freqs:
            value += math.sin(2 * math.pi * (f + glide * t) * t)
        value /= max(1, len(freqs))
        if noise:
            value = value * (1 - noise) + (rng.random() * 2 - 1) * noise
        samples.append(value * volume * env(t, attack, release, length))
    return samples


def concat(*parts, gap=0.0):
    out = []
    for part in parts:
        out += part + [0.0] * int(RATE * gap)
    return out


def rumble(length, volume=0.6):
    out, low = [], 0.0
    for i in range(int(RATE * length)):
        t = i / RATE
        low = low * 0.995 + (rng.random() * 2 - 1) * 0.005
        swell = min(1.0, t / (length * 0.6)) * env(t, 0.01, 0.4, length)
        out.append((low * 18 + 0.25 * math.sin(2 * math.pi * 55 * t)) * swell * volume)
    return out


def whoosh(length, volume=0.4):
    out, low = [], 0.0
    for i in range(int(RATE * length)):
        t = i / RATE
        k = 0.02 + 0.2 * math.sin(math.pi * t / length)
        low = low * (1 - k) + (rng.random() * 2 - 1) * k
        out.append(low * volume * 3 * math.sin(math.pi * t / length))
    return out


def write(name, samples):
    os.makedirs(OUT, exist_ok=True)
    path = os.path.join(OUT, f"{name}.wav")
    peak = max(1e-6, max(abs(s) for s in samples))
    scale = min(1.0, 0.9 / peak)
    with wave.open(path, "wb") as w:
        w.setnchannels(1)
        w.setsampwidth(2)
        w.setframerate(RATE)
        w.writeframes(b"".join(struct.pack("<h", int(max(-1, min(1, s * scale)) * 32767)) for s in samples))
    print(path, f"{len(samples) / RATE:.2f}s")


SOUNDS = {
    "SFX_TD_UI_Hover": tone([1800], 0.03, 0.25, release=0.02),
    "SFX_TD_UI_Confirm": concat(tone([880, 1320], 0.05, 0.5), tone([1175, 1760], 0.07, 0.5)),
    "SFX_TD_UI_Back": tone([392, 523], 0.08, 0.45, glide=-600),
    "SFX_TD_UI_Locked": tone([140], 0.12, 0.6, noise=0.3, release=0.08),
    "SFX_TD_CardFlip": whoosh(0.32),
    "SFX_TD_RadioIn": concat(tone([1500], 0.06, 0.35, noise=0.25), tone([1500], 0.06, 0.35, noise=0.25), tone([2000], 0.1, 0.35, noise=0.2), gap=0.04),
    # Calm alarm: slow two-tone, no harsh square waves (never scary, design.md §11).
    "SFX_TD_Alarm": concat(tone([660], 0.28, 0.45, attack=0.04, release=0.1), tone([523], 0.32, 0.45, attack=0.04, release=0.15), gap=0.08),
    "SFX_TD_Stamp": concat(tone([90], 0.09, 0.8, noise=0.5, release=0.07), tone([600], 0.05, 0.2)),
    "SFX_TD_LiftOff": rumble(1.8),
    "SFX_TD_SystemUp": tone([523, 659, 784], 0.25, 0.4, attack=0.02, release=0.15, glide=200),
    "SFX_TD_SystemDown": tone([392, 311], 0.3, 0.4, attack=0.02, release=0.2, glide=-150),
}

for sound_name, sound in SOUNDS.items():
    write(sound_name, sound)
