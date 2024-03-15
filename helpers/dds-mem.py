import numpy as np

a = 2**23 - 2

x = (np.arange(2048) + 0.5) / 4096
lut = np.round(a * np.cos(np.pi * x)).astype(np.uint32)

print("\n".join([format(v, "06x") for v in lut]))
