import os
import sys
import numpy as np
import scipy.signal
import matplotlib.pyplot as plt
from pedalboard import load_plugin, Pedalboard

vst_path = os.path.abspath(r"build/BoratoEq_artefacts/Release/VST3/BORATO EQ.vst3/Contents/x86_64-win/BORATO EQ.vst3")
if not os.path.exists(vst_path):
    print("VST3 not found!")
    sys.exit(1)

plugin = load_plugin(vst_path)

sample_rate = 44100
duration = 1.0 # 1 second
t = np.arange(int(sample_rate * duration)) / sample_rate
# Impulse
impulse = np.zeros(int(sample_rate * duration))
impulse[0] = 1.0

# Process stereo impulse
audio_in = np.vstack((impulse, impulse)).astype(np.float32)

def measure_and_plot(name, p_idx, ax):
    # Apply program (preset) directly
    plugin.program = name
    
    board = Pedalboard([plugin])
    audio_out = board(audio_in, sample_rate)
    
    w, h = scipy.signal.freqz(audio_out[0], 1, worN=8192, fs=sample_rate)
    mag = 20 * np.log10(np.abs(h) + 1e-10)
    
    freqs = w * sample_rate / (2 * np.pi)
    
    ax.plot(freqs, mag, label=name)
    ax.set_xscale('log')
    ax.set_xlim(20, 20000)
    ax.set_ylim(-15, 15)
    ax.grid(True, which="both", ls="-", alpha=0.5)
    ax.set_title(name)
    
    # Let's print out what iron parameter is currently set to inside pedalboard
    # Wait, getting parameter after changing program might not reflect immediately? Let's check
    print(f"[{name}] Iron: {plugin.iron}, Tube: {plugin.tube}")

fig, axes = plt.subplots(2, 3, figsize=(15, 10))
axes = axes.flatten()

# Factory presets indices:
# 0: Default
# 1: Vocal Air
# 2: Bus Glue
# 3: Low-End Weight
# 4: Vintage Dark
# 5: Modern Open
# 6: Master Polish

configs = [
    ("Default", 0),
    ("Vocal Air", 1),
    ("Bus Glue", 2),
    ("Low-End Weight", 3),
    ("Vintage Dark", 4),
    ("Modern Open", 5)
]

for i, (name, idx) in enumerate(configs):
    if i < len(axes):
        measure_and_plot(name, idx, axes[i])

plt.tight_layout()
plt.savefig("spectral_analysis.png")
print("Saved spectral_analysis.png")
