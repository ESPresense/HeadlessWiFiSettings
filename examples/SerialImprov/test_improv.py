#!/usr/bin/env python3
"""Minimal SerialImprov exerciser.

Usage:
    python3 test_improv.py /dev/cu.usbserial-XXXX [SSID] [PASSWORD]

If SSID/PASSWORD are omitted the script only issues IDENTIFY/GET_STATE and
prints any responses.
"""

from __future__ import annotations

import argparse
import os
import sys
import time
from pathlib import Path
from typing import List, Tuple

import serial

IMPROV_HEADER = b"IMPROV"
IMPROV_SERIAL_VERSION = 1
TYPE_CURRENT_STATE = 0x01
TYPE_ERROR_STATE = 0x02
TYPE_RPC_COMMAND = 0x03
TYPE_RPC_RESULT = 0x04

COMMAND_WIFI_SETTINGS = 0x01
COMMAND_IDENTIFY = 0x02
COMMAND_GET_STATE = 0x02
COMMAND_GET_DEVICE_INFO = 0x03

STATE_AWAITING_AUTHORIZATION = 0x01
STATE_AUTHORIZED = 0x02
STATE_PROVISIONING = 0x03
STATE_PROVISIONED = 0x04

STATE_NAMES = {
    STATE_AWAITING_AUTHORIZATION: "AWAITING_AUTHORIZATION",
    STATE_AUTHORIZED: "AUTHORIZED",
    STATE_PROVISIONING: "PROVISIONING",
    STATE_PROVISIONED: "PROVISIONED",
}

ERROR_NAMES = {
    0x00: "NONE",
    0x01: "INVALID_RPC",
    0x02: "UNKNOWN_RPC",
    0x03: "UNABLE_TO_CONNECT",
    0x04: "NOT_AUTHORIZED",
    0xFF: "UNKNOWN",
}


def load_env_file(filename: str = ".env") -> None:
    path = Path(filename)
    if not path.exists():
        return
    for raw_line in path.read_text().splitlines():
        line = raw_line.strip()
        if not line or line.startswith("#") or "=" not in line:
            continue
        key, value = line.split("=", 1)
        key = key.strip()
        value = value.strip().strip('"').strip("'")
        os.environ.setdefault(key, value)


def checksum(data: bytes) -> int:
    return sum(data) & 0xFF


def build_frame(packet_type: int, payload: bytes) -> bytes:
    frame = bytearray(IMPROV_HEADER)
    frame.append(IMPROV_SERIAL_VERSION)
    frame.append(packet_type)
    frame.append(len(payload))
    frame.extend(payload)
    frame.append(checksum(frame))
    return bytes(frame)


def build_rpc(command: int, payload: bytes) -> bytes:
    inner = bytearray([command, len(payload)])
    inner.extend(payload)
    return build_frame(TYPE_RPC_COMMAND, inner)


def wifi_payload(ssid: str, password: str) -> bytes:
    body = bytearray()
    body.append(len(ssid))
    body.extend(ssid.encode())
    body.append(len(password))
    body.extend(password.encode())
    return bytes(body)


def identify_packet() -> bytes:
    return build_rpc(COMMAND_IDENTIFY, b"")


def device_info_packet() -> bytes:
    return build_rpc(COMMAND_GET_DEVICE_INFO, b"")


def wifi_packet(ssid: str, password: str) -> bytes:
    return build_rpc(COMMAND_WIFI_SETTINGS, wifi_payload(ssid, password))


def current_state_packet() -> bytes:
    return build_rpc(COMMAND_GET_STATE, b"")


def parse_packets(buffer: bytearray) -> List[Tuple[int, bytes]]:
    packets: List[Tuple[int, bytes]] = []
    while True:
        start = buffer.find(IMPROV_HEADER)
        if start == -1 or len(buffer) < start + 9:
            break
        version = buffer[start + 6]
        if version != IMPROV_SERIAL_VERSION:
            del buffer[: start + 1]
            continue
        pkt_type = buffer[start + 7]
        length = buffer[start + 8]
        end = start + 9 + length
        if len(buffer) < end + 1:
            break
        payload = bytes(buffer[start + 9 : end])
        cksum = buffer[end]
        if checksum(buffer[start:end]) != cksum:
            del buffer[: start + 1]
            continue
        packets.append((pkt_type, payload))
        del buffer[: end + 1]
    return packets


def decode_rpc_result(payload: bytes) -> Tuple[int, list[str]] | None:
    if len(payload) < 2:
        return None
    command = payload[0]
    total_len = payload[1]
    idx = 2
    values: list[str] = []
    while idx < len(payload):
        length = payload[idx]
        idx += 1
        values.append(payload[idx : idx + length].decode("utf-8", errors="replace"))
        idx += length
    return command, values


def print_packets(packets: List[Tuple[int, bytes]]) -> None:
    for pkt_type, payload in packets:
        if pkt_type == TYPE_CURRENT_STATE and payload:
            state = STATE_NAMES.get(payload[0], f"0x{payload[0]:02x}")
            print(f"State update: {state}")
        elif pkt_type == TYPE_ERROR_STATE and payload:
            err = ERROR_NAMES.get(payload[0], f"0x{payload[0]:02x}")
            print(f"Error: {err}")
        elif pkt_type == TYPE_RPC_RESULT:
            decoded = decode_rpc_result(payload)
            if decoded:
                command, values = decoded
                if command == COMMAND_GET_DEVICE_INFO and len(values) >= 4:
                    fw, ver, chip, name = values[:4]
                    print(f"Device info: firmware='{fw}' version='{ver}' chip='{chip}' name='{name}'")
                elif command == COMMAND_IDENTIFY:
                    print(f"Identify RPC response: {values}")
                else:
                    print(f"RPC result (cmd 0x{command:02x}): {values}")
            else:
                print(f"RPC result: {payload.hex() if payload else '(empty)'}")
        else:
            print(f"Packet type 0x{pkt_type:02x}: {payload.hex()}")


def read_for(
    ser: serial.Serial,
    seconds: float,
    buffer: bytearray,
) -> List[Tuple[int, bytes]]:
    deadline = time.time() + seconds
    captured: List[Tuple[int, bytes]] = []
    while time.time() < deadline:
        data = ser.read(ser.in_waiting or 1)
        if data:
            buffer.extend(data)
            if b"IMPROV" not in data:
                try:
                    sys.stdout.write(data.decode(errors="ignore"))
                    sys.stdout.flush()
                except Exception:
                    sys.stdout.buffer.write(data)
                    sys.stdout.flush()
            sys.stdout.write(data.decode(errors="ignore"))
            packets = parse_packets(buffer)
            if packets:
                captured.extend(packets)
                print_packets(packets)
        time.sleep(0.01)
    return captured


def wait_for_provisioned(ser: serial.Serial, buffer: bytearray, timeout: float) -> bool:
    deadline = time.time() + timeout
    while time.time() < deadline:
        packets = read_for(ser, 0.5, buffer)
        for pkt_type, payload in packets:
            if pkt_type == TYPE_CURRENT_STATE and payload:
                if payload[0] == STATE_PROVISIONED:
                    print("Provisioning complete.")
                    return True
            if pkt_type == TYPE_ERROR_STATE and payload:
                print("Provisioning failed (error state).")
                return False
        time.sleep(0.05)
    print("Timed out waiting for STATE_PROVISIONED.")
    return False


def main() -> int:
    load_env_file()
    parser = argparse.ArgumentParser(description="SerialImprov exerciser")
    parser.add_argument("port", help="Serial port, e.g. /dev/cu.usbserial-XXXX")
    parser.add_argument("ssid", nargs="?", help="Wi-Fi SSID (optional if SERIAL_IMPROV_SSID env set)")
    parser.add_argument("password", nargs="?", default="", help="Wi-Fi password (optional if SERIAL_IMPROV_PASSWORD env set)")
    parser.add_argument("--no-reset", action="store_true", help="Skip toggling DTR/RTS on open")
    args = parser.parse_args()

    port = args.port or os.environ.get("SERIAL_IMPROV_PORT")
    if not port:
        print("No serial port provided (argument or SERIAL_IMPROV_PORT env required).")
        return 1

    ssid = args.ssid if args.ssid is not None else os.environ.get("SERIAL_IMPROV_SSID")
    password = args.password if args.password else os.environ.get("SERIAL_IMPROV_PASSWORD", "")

    try:
        ser = serial.Serial(port, 115200, timeout=0.1)
    except serial.SerialException as err:
        print(f"Could not open {port}: {err}")
        return 1

    if not args.no_reset:
        ser.dtr = False
        ser.rts = False
        time.sleep(0.1)
        ser.dtr = True
        ser.rts = True

    buffer = bytearray()

    print("Reading boot/state output...")
    read_for(ser, 2.0, buffer)

    print("Sending IDENTIFY")
    ser.write(identify_packet())
    read_for(ser, 1.0, buffer)

    print("Requesting device info")
    ser.write(device_info_packet())
    read_for(ser, 1.0, buffer)

    print("Requesting current state")
    ser.write(current_state_packet())
    read_for(ser, 1.0, buffer)

    if ssid is not None:
        print(f"Sending Wi-Fi credentials for '{ssid}'")
        ser.write(wifi_packet(ssid, password))
        if not wait_for_provisioned(ser, buffer, timeout=60.0):
            ser.close()
            return 2

    ser.close()
    return 0


if __name__ == "__main__":
    sys.exit(main())
