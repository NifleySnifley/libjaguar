import math
import ctypes
import ctypes.wintypes
import os
from enum import Enum
from time import sleep
from sys import platform

ENCODER_1CH = 0
ENCODER_1CH_INV = 2
ENCODER_QUAD = 3
POSITION_ENCODER = 0
POSITION_POTENTIOMETER = 1
FORWARD_LIMIT_REACHED = 0
REVERSE_LIMIT_REACHED = 1
CURRENT_FAULT = 0
TEMPERATURE_FAULT = 1
BUS_VOLTAGE_FAULT = 2


# TODO: *NIX and socketCAN implementations
class _CANConnection_Internal(ctypes.Structure):
    _fields_ = [('hSerial', ctypes.wintypes.HANDLE)]

P = "../build.liblibjaguar.dll" if platform == "win32" else "../build/liblibjaguar.so"
P = os.path.abspath(P)
LIB = ctypes.CDLL(P)


def percent_to_fixed16(percent):
    LIB.percent_to_fixed16.restype = ctypes.c_uint16
    return LIB.percent_to_fixed16(percent)


def fixed16_to_float(fixed):
    LIB.fixed16_to_float.restype = ctypes.c_float
    return LIB.fixed16_to_float(fixed)


def fixed32_to_float(fixed):
    LIB.fixed32_to_float.restype = ctypes.c_float
    return LIB.fixed32_to_float(fixed)


def float_to_fixed16(fixed):
    LIB.float_to_fixed16.argtypes = (ctypes.c_float,)
    LIB.float_to_fixed16.restype = ctypes.c_uint16
    return LIB.float_to_fixed16(fixed)


def float_to_fixed32(fixed):
    LIB.float_to_fixed32.argtypes = (ctypes.c_float,)
    LIB.float_to_fixed32.restype = ctypes.c_uint32
    return LIB.float_to_fixed32(fixed)


class JaguarCAN():
    def __init__(self, port):
        self.connection = _CANConnection_Internal()
        self.cptr = ctypes.pointer(self.connection)
        self.port = port

    def connect(self):
        s = ctypes.create_string_buffer(bytes(self.port, 'ascii'))
        return LIB.open_can_connection(self.cptr, s)

    def reset(self, dvc):
        LIB.sys_reset.argtypes = (ctypes.POINTER(
            _CANConnection_Internal), ctypes.c_uint8)
        return LIB.sys_reset(self.cptr, dvc)

    def halt(self, dvc):
        return LIB.sys_halt(self.cptr, dvc)

    def resume(self, dvc):
        return LIB.sys_resume(self.cptr, dvc)

    def heartbeat(self, dvc):
        return LIB.sys_heartbeat(self.cptr, dvc)

    def voltage_enable(self, dvc):
        return LIB.voltage_enable(self.cptr, dvc)

    def voltage_disable(self, dvc):
        return LIB.voltage_disable(self.cptr, dvc)

    def voltage_set(self, dvc, percent: float):
        v = percent_to_fixed16(ctypes.c_float(max(-1.0, min(1.0, percent))))
        return LIB.voltage_set(self.cptr, dvc, v)

    def voltage_ramp(self, dvc, vramp):
        v = percent_to_fixed16(max(-1.0, min(1.0, vramp)))
        return LIB.voltage_ramp(self.cptr, dvc, v)

    def status_speed(self, dvc) -> float:
        """Returns speed in rotations/second

        Args:
            dvc (int): device number

        Returns:
            float: speed in rotations/second
        """
        v = ctypes.c_uint32()
        LIB.status_speed(self.cptr, dvc, ctypes.pointer(v))
        return fixed32_to_float(v) / 60.0

    def status_temperature(self, dvc) -> float:
        """Returns speed in rotations/second

        Args:
            dvc (int): device number

        Returns:
            float: speed in rotations/second
        """
        v = ctypes.c_uint16()
        LIB.status_temperature(self.cptr, dvc, ctypes.pointer(v))
        return fixed16_to_float(v)

    def status_position(self, dvc) -> float:
        """Returns posotion in rotations

        Args:
            dvc (int): device number

        Returns:
            float: position in rotations
        """
        v = ctypes.c_uint32()
        LIB.status_position(self.cptr, dvc, ctypes.pointer(v))
        return fixed32_to_float(v)

    def status_output_percent(self, dvc) -> float:
        v = ctypes.c_int16()
        LIB.status_output_percent(self.cptr, dvc, ctypes.pointer(v))
        return v.value / (2**15)  # fixed16_to_float(v)

    def status_current(self, dvc) -> float:
        v = ctypes.c_int16()
        LIB.status_current(self.cptr, dvc, ctypes.pointer(v))
        return fixed16_to_float(v)

    def config_encoder_cpr(self, dvc, cpr):
        return LIB.config_encoder_lines(self.cptr, dvc, cpr)

    def position_set_ref(self, dvc, t=POSITION_ENCODER):
        return LIB.position_set_ref(self.cptr, dvc, t)

    def reset_position(self, dvc, reset_to=0.0):
        LIB.position_enable(self.cptr, dvc, float_to_fixed32(reset_to))

    def position_ref_encoder(self, dvc):
        return LIB.position_ref_encoder(self.cptr, dvc)

    def speed_set_ref(self, dvc, t=ENCODER_QUAD):
        return LIB.speed_set_ref(self.cptr, dvc, t)

    def disconnect(self):
        return LIB.close_can_connection(self.cptr)

    def voltcomp_enable(self, dvc):
        return LIB.voltcomp_enable(self.cptr, dvc)

    def voltcomp_disable(self, dvc):
        return LIB.voltcomp_disable(self.cptr, dvc)

    def voltcomp_ramp(self, dvc, vramp):
        v = float_to_fixed16(vramp)
        return LIB.voltcomp_ramp(self.cptr, dvc, v)

    def voltcomp_set(self, dvc, voltage: float):
        v = float_to_fixed16(voltage)
        return LIB.voltcomp_set(self.cptr, dvc, v)

    def voltcomp_rate(self, dvc, vrate):
        v = float_to_fixed16(vrate)
        return LIB.voltcomp_comp_ramp(self.cptr, dvc, v)


class JaguarWrapper():
    def __init__(self, bus: JaguarCAN, id: int, encoder_cpr=2048, vcomp=False, inverted=False):
        self.id = id
        self.bus = bus
        self.cpr = encoder_cpr
        self.vcomp = vcomp
        self.inverted = inverted

    def enable(self):
        self.bus.reset_position(self.id)

        if (self.vcomp):
            self.bus.voltcomp_enable(self.id)
            self.bus.voltcomp_ramp(self.id, 0.0)
            self.bus.voltcomp_rate(self.id, 0.0)
        else:
            self.bus.voltage_enable(self.id)
        self.bus.config_encoder_cpr(self.id, self.cpr)
        self.bus.position_ref_encoder(self.id)
        self.bus.speed_set_ref(self.id, ENCODER_QUAD)

    def heartbeat(self):
        self.bus.heartbeat(self.id)

    def set(self, target: float):
        target *= (-1.0 if self.inverted else 1.0)
        if (self.vcomp):
            self.bus.voltcomp_set(
                self.id, target)
        else:
            self.bus.voltage_set(self.id, target)

    def get(self):
        return self.bus.status_output_percent(self.id) * (-1.0 if self.inverted else 1.0)

    def get_position(self):
        return self.bus.status_position(self.id)

    def get_speed(self):
        return self.bus.status_speed(self.id)

    def get_current(self):
        return self.bus.status_current(self.id)
