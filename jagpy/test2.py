from libjaguar import percent_to_fixed16, fixed16_to_float, float_to_fixed16
from random import random

# print(fixed16_to_float(float_to_fixed16(-1.5)))

for i in range(20):
    n = (2*random()-1.0) * 100
    print(f"{n:0.3f} -> {fixed16_to_float(float_to_fixed16(n)):0.3f}")
