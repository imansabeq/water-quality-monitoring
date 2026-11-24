import json
import serial
import paho.mqtt.client as mqtt

COM_PORT = "COM9"
BAUD_RATE = 9600
MQTT_HOST = "localhost"
MQTT_PORT = 1883
MQTT_TOPIC = "water/telemetry"

try:
    client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
except AttributeError:
    client = mqtt.Client()

client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
client.loop_start()

print(f"Connected to MQTT ({MQTT_HOST}:{MQTT_PORT}). Listening on {COM_PORT}...")

with serial.Serial(COM_PORT, BAUD_RATE, timeout=2) as ser:
    while True:
        line = ser.readline().decode("utf-8", errors="ignore").strip()
        if not line:
            continue
        if not line.startswith("{"):
            print(f"[SKIP] {line}")
            continue
        try:
            json.loads(line)
            client.publish(MQTT_TOPIC, line)
            print(f"[MQTT ->] {line}")
        except json.JSONDecodeError:
            print(f"[WARN] Invalid JSON: {line}")