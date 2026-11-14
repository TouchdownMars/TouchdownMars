# Copyright 2026 Team Touchdown. Licensed under the Apache License, Version 2.0.
"""Synthesize the game's UI and scene sounds (sound tokens) as 16-bit mono WAV files.

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
    # Calm alarm: slow two-tone, no harsh square waves (never scary).
    "SFX_TD_Alarm": concat(tone([660], 0.28, 0.45, attack=0.04, release=0.1), tone([523], 0.32, 0.45, attack=0.04, release=0.15), gap=0.08),
    "SFX_TD_Stamp": concat(tone([90], 0.09, 0.8, noise=0.5, release=0.07), tone([600], 0.05, 0.2)),
    "SFX_TD_LiftOff": rumble(1.8),
    "SFX_TD_SystemUp": tone([523, 659, 784], 0.25, 0.4, attack=0.02, release=0.15, glide=200),
    "SFX_TD_SystemDown": tone([392, 311], 0.3, 0.4, attack=0.02, release=0.2, glide=-150),
}

for sound_name, sound in SOUNDS.items():
    write(sound_name, sound)

# Hands-on work and surface ambience (14 Nov 2026): also synthesized here, team-made.
def decay(length, partials, tau, volume=0.6, click=0.0):
    out = []
    for i in range(int(RATE * length)):
        t = i / RATE
        v = sum(a * math.sin(2 * math.pi * f * t) for f, a in partials) / max(1, len(partials))
        if click and t < 0.006:
            v += (rng.random() * 2 - 1) * click
        out.append(v * volume * math.exp(-t / tau))
    return out


def noise_burst(length, tone_hz=0.0, cutoff=0.3, volume=0.5, tau=0.05):
    out, low = [], 0.0
    for i in range(int(RATE * length)):
        t = i / RATE
        low = low * (1 - cutoff) + (rng.random() * 2 - 1) * cutoff
        v = low * 2 + (0.4 * math.sin(2 * math.pi * tone_hz * t) if tone_hz else 0.0)
        out.append(v * volume * math.exp(-t / tau))
    return out


def motor(length, base, volume=0.45):
    out, phase = [], 0.0
    for i in range(int(RATE * length)):
        t = i / RATE
        f = base * (1 + 0.04 * math.sin(2 * math.pi * 7 * t)) * (0.7 + 0.3 * min(1.0, t / 0.15))
        phase += 2 * math.pi * f / RATE
        v = sum(math.sin(k * phase) / k for k in range(1, 7)) * 0.5 + (rng.random() * 2 - 1) * 0.15
        out.append(v * volume * env(t, 0.05, 0.12, length))
    return out


def loop_noise(length, cutoff, volume, wobble_hz, hum=None):
    n = int(RATE * length)
    out, low, low2 = [], 0.0, 0.0
    for i in range(n):
        t = i / RATE
        low = low * (1 - cutoff) + (rng.random() * 2 - 1) * cutoff
        low2 = low2 * (1 - cutoff) + low * cutoff
        gust = 0.6 + 0.4 * math.sin(2 * math.pi * wobble_hz * t) * math.sin(2 * math.pi * wobble_hz * 0.37 * t + 1.3)
        v = low2 * 6 * gust
        if hum:
            v += sum(a * math.sin(2 * math.pi * f * t) for f, a in hum)
        out.append(v * volume)
    # Seamless loop: cross-fade the last 0.5 s into the start.
    fade = int(RATE * 0.5)
    for i in range(fade):
        a = i / fade
        out[i] = out[i] * a + out[n - fade + i] * (1 - a)
    return out[: n - fade]


WORK_SOUNDS = {
    "SFX_TD_Work_Hammer": decay(0.35, [(520, 1.0), (1310, 0.7), (2470, 0.5), (3900, 0.3)], 0.08, 0.8, click=0.8),
    "SFX_TD_Work_Ratchet": concat(*[noise_burst(0.012, 3200, 0.6, 0.6, 0.004) for _ in range(5)], gap=0.03),
    "SFX_TD_Work_Plug": concat(noise_burst(0.02, 2500, 0.7, 0.6, 0.006), decay(0.12, [(110, 1.0), (220, 0.4)], 0.04, 0.8)),
    "SFX_TD_Work_Latch": concat(decay(0.06, [(1800, 1.0), (2900, 0.6)], 0.015, 0.7, click=0.6), decay(0.08, [(900, 1.0), (1500, 0.5)], 0.02, 0.6, click=0.5), gap=0.05),
    "SFX_TD_Work_Brush": whoosh(0.26, 0.5),
    "SFX_TD_Work_Drill": motor(0.6, 140),
    "SFX_TD_Work_Scoop": concat(noise_burst(0.18, 0, 0.15, 0.7, 0.08), noise_burst(0.12, 0, 0.08, 0.6, 0.05), gap=0.02),
    "SFX_TD_Work_Scan": concat(tone([1900], 0.05, 0.35), tone([2400], 0.07, 0.35), gap=0.03),
    "SFX_TD_Work_Switch": decay(0.05, [(2200, 1.0), (3400, 0.5)], 0.01, 0.7, click=0.7),
    "SFX_TD_Work_Done": decay(0.9, [(1046, 1.0), (1568, 0.6), (2093, 0.3)], 0.3, 0.6),
    "SFX_TD_Ambience_MarsWind": loop_noise(12.5, 0.02, 0.5, 0.11),
    "SFX_TD_Ambience_BaseHum": loop_noise(6.5, 0.05, 0.08, 0.5, hum=[(60, 0.25), (120, 0.12), (180, 0.05)]),
}

for sound_name, sound in WORK_SOUNDS.items():
    write(sound_name, sound)
