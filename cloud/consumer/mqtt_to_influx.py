import json
import os
import time
import paho.mqtt.client as mqtt
from influxdb_client import InfluxDBClient, Point
from influxdb_client.client.write_api import SYNCHRONOUS

MQTT_HOST = os.getenv("MQTT_HOST", "mosquitto")
MQTT_PORT = int(os.getenv("MQTT_PORT", "1883"))
MQTT_TOPIC = os.getenv("MQTT_TOPIC", "water/telemetry")

INFLUX_URL = os.getenv("INFLUX_URL", "http://influxdb:8086")
INFLUX_TOKEN = os.getenv("INFLUX_TOKEN", "water-quality-dev-token")
INFLUX_ORG = os.getenv("INFLUX_ORG", "water-iot")
INFLUX_BUCKET = os.getenv("INFLUX_BUCKET", "water_quality")

influx = InfluxDBClient(url=INFLUX_URL, token=INFLUX_TOKEN, org=INFLUX_ORG)
write_api = influx.write_api(write_options=SYNCHRONOUS)


def on_connect(client, userdata, flags, reason_code, properties):
    print(f"[MQTT] Connected to broker (code={reason_code}), subscribing to {MQTT_TOPIC}")
    client.subscribe(MQTT_TOPIC)


def on_message(client, userdata, msg):
    raw = msg.payload.decode("utf-8", errors="ignore").strip()
    try:
        data = json.loads(raw)
        reasons = ", ".join(data.get("reasons", [])) or "None"
        point = (
            Point("water_quality")
            .tag("node", "proteus_uno1")
            .field("ph", float(data["ph"]))
            .field("chlorine", float(data["chlorine"]))
            .field("turbidity", float(data["turbidity"]))
            .field("tds", float(data["tds"]))
            .field("safe", 1 if data.get("safe") else 0)
            .field("reasons", reasons)
        )
        write_api.write(bucket=INFLUX_BUCKET, org=INFLUX_ORG, record=point)
        print(f"[InfluxDB] Saved: pH={data['ph']} | Cl={data['chlorine']} | Turb={data['turbidity']} | TDS={data['tds']} | safe={data['safe']}")
    except Exception as e:
        print(f"[ERROR] Failed to process message '{raw}': {e}")


client = mqtt.Client(mqtt.CallbackAPIVersion.VERSION2)
client.on_connect = on_connect
client.on_message = on_message

while True:
    try:
        client.connect(MQTT_HOST, MQTT_PORT, keepalive=60)
        client.loop_forever()
    except Exception as e:
        print(f"[MQTT] Waiting for broker... ({e})")
        time.sleep(3)