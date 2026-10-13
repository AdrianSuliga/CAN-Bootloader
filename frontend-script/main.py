import os
import time
import struct
import subprocess
import sys
import paho.mqtt.client as mqtt

BROKER = "broker.hivemq.com"
PORT = 1883

TOPIC = "system/gateway_board/subscribe/new_app"

def get_arg_paths():
    if len(sys.argv) not in [2, 3]:
        print('Error: Wrong number of arguments (expected 1 or 2).', file=sys.stderr)
        print('Either provide the path to the zephyr workspace, '
              'or paths to the elf and the bin.', file=sys.stderr)
        exit(1)
    elif len(sys.argv) == 2:
        zephyr_workspace = sys.argv[1]
        artifact_base = zephyr_workspace + '/build/zephyr/zephyr'
        return artifact_base + '.elf', artifact_base + '.bin'
    else:
        return sys.argv[1], sys.argv[2]

elf_path, bin_path = get_arg_paths()

print("Converting ELF to BIN...")

subprocess.run(
    [
        "arm-none-eabi-objcopy",
        "-O",
        "binary",
        elf_path,
        bin_path,
    ],
    check=True,
)

if not os.path.exists(bin_path):
    raise FileNotFoundError("Failed to create binary")

# Two 2-byte numbers
number1 = 0x400
number2 = 0x401

# < = little-endian
# H = unsigned 2-byte integer
header = struct.pack("<HH", number1, number2)

# Read binary firmware
with open(bin_path, "rb") as f:
    firmware_data = f.read()

print(f"Firmware size: {len(firmware_data)} bytes")

client = mqtt.Client()

print(f"Connecting to {BROKER}:{PORT}")
client.connect(BROKER, PORT, 60)

client.loop_start()

print(f"Publishing firmware to topic: {TOPIC}")

result = client.publish(
    TOPIC,
    payload=header + firmware_data,
    qos=1,
)

result.wait_for_publish()

if result.rc == mqtt.MQTT_ERR_SUCCESS:
    print("Firmware published successfully")
else:
    print("Failed to publish firmware")

time.sleep(1)

client.loop_stop()
client.disconnect()

print("Done")
