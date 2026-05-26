import os
import time
import subprocess
import paho.mqtt.client as mqtt

BROKER = "broker.hivemq.com"
PORT = 1883

TOPIC = "system/gateway_board/subscribe/new_app"

ELF_PATH = "/home/arima/zephyrproject/CAN-Bootloader/user-application/build/Debug/user-application.elf"
BIN_PATH = "/tmp/user-application.bin"

print("Converting ELF to BIN...")

subprocess.run(
    [
        "arm-none-eabi-objcopy",
        "-O",
        "binary",
        ELF_PATH,
        BIN_PATH,
    ],
    check=True,
)

if not os.path.exists(BIN_PATH):
    raise FileNotFoundError("Failed to create binary")

# Read binary firmware
with open(BIN_PATH, "rb") as f:
    firmware_data = f.read()

print(f"Firmware size: {len(firmware_data)} bytes")

client = mqtt.Client()

print(f"Connecting to {BROKER}:{PORT}")
client.connect(BROKER, PORT, 60)

client.loop_start()

print(f"Publishing firmware to topic: {TOPIC}")

result = client.publish(
    TOPIC,
    payload=firmware_data,
    qos=0,
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
