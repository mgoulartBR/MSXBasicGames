"""Prints the lookup tables embedded in src/sp_logic.c."""
import math
print('sin_q:', ', '.join(str(round(255 * math.sin(math.pi / 2 * i / 64))) for i in range(65)))
print('inv_tab:', ', '.join(str(round(4096 / d)) for d in range(16, 129)))
