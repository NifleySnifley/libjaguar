import time
from libjaguar import JaguarCAN, JaguarWrapper, fixed16_to_float
import argparse

parser = argparse.ArgumentParser(
    prog='Test libjaguar.py',
    description='Runs a jaguar controller at the specified speed')
parser.add_argument(
    "PORT", help="serial port to communicate with", type=str)
parser.add_argument("id", type=int,
                    help="CAN ID of the controller to set")
parser.add_argument("--throttle", '-t', type=float, default=0.0)
args = parser.parse_args()

can = JaguarCAN(args.PORT)
can.connect()

j = JaguarWrapper(can, args.id, vcomp=True, inverted=True)
j.enable()

stime = time.time()
while True:
    j.heartbeat()
    j.set(args.throttle)
    print(
        f"Throt: {j.get(): .2f}, Speed: {j.get_speed(): .2f}, Pos: {j.get_position(): .2f}, Cur: {j.get_current():0.2f}")
    # print(j.get_speed())

# print(jag.status_speed(D))
# jag.voltage_set(D, 0.1)

can.disconnect()
