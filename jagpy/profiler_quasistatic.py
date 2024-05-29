import time
from libjaguar import JaguarCAN, JaguarWrapper, fixed16_to_float
import argparse
import pickle
import numpy as np

parser = argparse.ArgumentParser(
    prog='Test libjaguar.py',
    description='Runs a jaguar controller at the specified speed')
parser.add_argument(
    "PORT", help="serial port to communicate with", type=str)
parser.add_argument("id", type=int,
                    help="CAN ID of the controller to set")
args = parser.parse_args()

can = JaguarCAN(args.PORT)
can.connect()

j = JaguarWrapper(can, args.id, vcomp=True, inverted=True)
j.enable()

vrange = np.linspace(0.0, 12.0, 2000)
res = np.zeros(vrange.shape)

for i, v in enumerate(list(vrange)):
    j.heartbeat()
    j.set(v)
    time.sleep(0.01)
    res[i] = j.get_speed()

with open('profile.pickle', 'wb') as f:
    # Pickle the 'data' dictionary using the highest protocol available.
    pickle.dump((vrange, res), f)

can.disconnect()
