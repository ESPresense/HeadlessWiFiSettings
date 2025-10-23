#!/usr/bin/env python3
"""
Test script for SerialImprov protocol
Based on: https://www.improv-wifi.com/serial/

Usage: python3 test_improv.py /dev/ttyUSB0
"""

import serial
import sys
import struct
import time

# Improv Serial Protocol Constants
IMPROV_SERIAL_VERSION = 1

# Command types
TYPE_CURRENT_STATE = 0x01
TYPE_ERROR_STATE = 0x02
TYPE_RPC_COMMAND = 0x03
TYPE_RPC_RESULT = 0x04

# RPC Commands
COMMAND_WIFI_SETTINGS = 0x01
COMMAND_IDENTIFY = 0x02

# States
STATE_AUTHORIZED = 0x01
STATE_AWAITING_AUTHORIZATION = 0x02
STATE_PROVISIONING = 0x03
STATE_PROVISIONED = 0x04

# Errors
ERROR_NONE = 0x00
ERROR_INVALID_RPC = 0x01
ERROR_UNKNOWN_RPC = 0x02
ERROR_UNABLE_TO_CONNECT = 0x03
ERROR_NOT_AUTHORIZED = 0x04
ERROR_UNKNOWN = 0xFF


def calculate_checksum(data):
    """Calculate checksum for Improv packet"""
    return sum(data) & 0xFF


def create_improv_packet(packet_type, data):
    """Create an Improv Serial packet"""
    length = len(data)
    packet = bytearray([
        0x49, 0x4D, 0x50, 0x52, 0x4F, 0x56,  # "IMPROV" header
        IMPROV_SERIAL_VERSION,
        packet_type,
        length
    ])
    packet.extend(data)
    checksum = calculate_checksum(packet[6:])  # Checksum from version onwards
    packet.append(checksum)
    return bytes(packet)


def parse_improv_response(ser, timeout=5.0):
    """Parse Improv response from serial"""
    start_time = time.time()
    buffer = bytearray()

    while time.time() - start_time < timeout:
        if ser.in_waiting:
            buffer.extend(ser.read(ser.in_waiting))

            # Look for IMPROV header
            header_pos = buffer.find(b'IMPROV')
            if header_pos != -1:
                # Check if we have enough data for header + type + length
                if len(buffer) >= header_pos + 9:
                    version = buffer[header_pos + 6]
                    packet_type = buffer[header_pos + 7]
                    length = buffer[header_pos + 8]

                    # Check if we have complete packet
                    if len(buffer) >= header_pos + 9 + length + 1:
                        data = buffer[header_pos + 9:header_pos + 9 + length]
                        checksum = buffer[header_pos + 9 + length]

                        # Verify checksum
                        calc_checksum = calculate_checksum(buffer[header_pos + 6:header_pos + 9 + length])

                        if calc_checksum == checksum:
                            # Remove processed packet from buffer
                            buffer = buffer[header_pos + 10 + length:]
                            return packet_type, data
                        else:
                            print(f"Checksum mismatch: expected {checksum}, got {calc_checksum}")
                            buffer = buffer[header_pos + 1:]
        time.sleep(0.01)

    return None, None


def send_wifi_settings(ser, ssid, password):
    """Send WiFi credentials via Improv"""
    # Encode strings with length prefix
    data = bytearray()
    data.append(COMMAND_WIFI_SETTINGS)
    data.append(len(ssid))
    data.extend(ssid.encode('utf-8'))
    data.append(len(password))
    data.extend(password.encode('utf-8'))

    packet = create_improv_packet(TYPE_RPC_COMMAND, data)
    print(f"Sending WiFi settings packet: {packet.hex()}")
    ser.write(packet)


def send_identify(ser):
    """Send identify command"""
    data = bytearray([COMMAND_IDENTIFY])
    packet = create_improv_packet(TYPE_RPC_COMMAND, data)
    print(f"Sending identify packet: {packet.hex()}")
    ser.write(packet)


def test_improv(port, ssid=None, password=None):
    """Test SerialImprov protocol"""
    print(f"Opening serial port {port}...")

    try:
        ser = serial.Serial(port, 115200, timeout=1)
        time.sleep(2)  # Wait for ESP32 to boot

        print("\n=== Reading initial state ===")
        packet_type, data = parse_improv_response(ser, timeout=2.0)
        if packet_type == TYPE_CURRENT_STATE:
            state = data[0] if data else None
            print(f"Current state: {state}")
            if state == STATE_AUTHORIZED:
                print("  Device is authorized and ready")
            elif state == STATE_AWAITING_AUTHORIZATION:
                print("  Device is awaiting authorization")
            elif state == STATE_PROVISIONING:
                print("  Device is provisioning")
            elif state == STATE_PROVISIONED:
                print("  Device is already provisioned")

        print("\n=== Sending IDENTIFY command ===")
        send_identify(ser)
        time.sleep(0.5)

        # Read any responses
        while True:
            packet_type, data = parse_improv_response(ser, timeout=1.0)
            if packet_type is None:
                break
            print(f"Response type: 0x{packet_type:02x}, data: {data.hex() if data else '(empty)'}")

        if ssid:
            print(f"\n=== Sending WiFi credentials ===")
            print(f"SSID: {ssid}")
            print(f"Password: {'*' * len(password) if password else '(empty)'}")

            send_wifi_settings(ser, ssid, password or "")

            print("\n=== Waiting for provisioning response ===")
            start_time = time.time()
            while time.time() - start_time < 30:
                packet_type, data = parse_improv_response(ser, timeout=1.0)

                if packet_type == TYPE_CURRENT_STATE:
                    state = data[0] if data else None
                    print(f"State update: {state}")
                    if state == STATE_PROVISIONED:
                        print("✓ Successfully provisioned!")

                        # Read redirect URL if present
                        if len(data) > 1:
                            url_length = data[1]
                            if len(data) >= 2 + url_length:
                                url = data[2:2+url_length].decode('utf-8')
                                print(f"  Redirect URL: {url}")
                        break
                    elif state == STATE_PROVISIONING:
                        print("  Still provisioning...")

                elif packet_type == TYPE_ERROR_STATE:
                    error = data[0] if data else ERROR_UNKNOWN
                    print(f"✗ Error: {error}")
                    break

                elif packet_type == TYPE_RPC_RESULT:
                    print(f"RPC Result: {data.hex() if data else '(empty)'}")

        ser.close()
        print("\n=== Test complete ===")

    except serial.SerialException as e:
        print(f"Error: {e}")
        return False

    return True


if __name__ == "__main__":
    if len(sys.argv) < 2:
        print("Usage: python3 test_improv.py <serial_port> [ssid] [password]")
        print("Example: python3 test_improv.py /dev/ttyUSB0 MyWiFi MyPassword")
        sys.exit(1)

    port = sys.argv[1]
    ssid = sys.argv[2] if len(sys.argv) > 2 else None
    password = sys.argv[3] if len(sys.argv) > 3 else None

    test_improv(port, ssid, password)
