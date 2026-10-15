import math
print(', '.join(str(round(255*math.sin(math.pi/2*i/64))) for i in range(65)))
