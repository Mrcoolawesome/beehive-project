#!/usr/bin/env python3
import evdev
import socket
import struct
import sys

# --- F' UDP Configuration ---
UDP_IP = "127.0.0.1"
UDP_PORT = 50000

# The name Linux assigns to the Wii Balance Board
BOARD_NAME = "Nintendo Wii Remote Balance Board"

def find_balance_board():
    """Iterate through all Linux input devices to find the Balance Board."""
    devices = [evdev.InputDevice(path) for path in evdev.list_devices()]
    for device in devices:
        if BOARD_NAME in device.name:
            print(f"Found Balance Board at {device.path}")
            return device
    return None

def main():
    # 1. Connect to the physical board
    dev = find_balance_board()
    if not dev:
        print(f"Error: Could not find '{BOARD_NAME}'. Is it paired via Bluetooth?")
        sys.exit(1)

    # 2. Setup the UDP socket to talk to F'
    sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
    print(f"Ready. Streaming weight data to F' at {UDP_IP}:{UDP_PORT}...")

    # Dictionary to hold the most recent value from each of the 4 sensors
    sensors = {}

    try:
        # 3. Listen to the kernel events indefinitely
        for event in dev.read_loop():
            
            # EV_ABS (Absolute) events carry the actual pressure sensor data
            if event.type == evdev.ecodes.EV_ABS:
                
                # Update the specific corner's value
                # event.code tells us which sensor (Top-Left, Bottom-Right, etc.)
                sensors[event.code] = event.value
                
                # Sum the 4 sensors for the raw total weight
                raw_weight = sum(sensors.values())
                
                # The Linux kernel driver usually reports the raw sum in decigrams (0.01 kg)
                # (You can tweak this multiplier if your specific kernel version differs)
                weight_kg = float(raw_weight / 100.0)
                
                # Pack the float into 4 Big-Endian bytes for F'
                byte_data = struct.pack('>f', weight_kg)
                
                # Send the packet
                sock.sendto(byte_data, (UDP_IP, UDP_PORT))
                
    except KeyboardInterrupt:
        print("\nExiting and closing socket...")
        sock.close()

if __name__ == "__main__":
    main()