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

vrange = np.linspace(0.0, 1.0, 100)
res = np.zeros((vrange.shape[0], 3))

j.set(0.0)
j.heartbeat()

V = 6.0

stime = time.time()
for i, v in enumerate(list(vrange)):
    t = time.time()-stime
    # j.heartbeat()
    j.set(V)
    # time.sleep(0.01)
    res[i][0] = t
    res[i][1] = j.get_speed()
    res[i][2] = j.get_position()

with open('profile_step.pickle', 'wb') as f:
    # Pickle the 'data' dictionary using the highest protocol available.
    pickle.dump((V, res), f)

can.disconnect()
