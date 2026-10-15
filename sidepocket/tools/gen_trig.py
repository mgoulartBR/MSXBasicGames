import math
print(", ".join(str(round(256*math.sin(2*math.pi*i/64))) for i in range(64)))
