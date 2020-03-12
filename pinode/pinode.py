import socket, json, asyncio, logging
import paho.mqtt.client as mqtt

DEVICE_NAME = socket.gethostname() or 'pinode'

class Sensor:
    def read(self):
        return {}
    def verbose(self, value):
        return ""

class PiNode:
    def __init__(self, server: str):
        mqttc = mqtt.Client()
        mqttc.connect(server)
        mqttc.loop_start()
        self.mqttc = mqttc

        self.device_name = socket.gethostname()

        self.sensors: Dict[str, Sensor] = {}
    
    def add_sensor(self, name: str, sensor: Sensor, poll_freq=10.0):
        if name not in self.sensors:
            self.sensors[name] = sensor
        asyncio.run(self._add_polling_loop(name, poll_freq))

    def make_sensor_topic(self, name: str) -> str:
        return 'device/{}/sensors/{}'.format(self.device_name, name)

    def publish_sensor_data(self, name: str):
        if name not in self.sensors:
            return
        sensor = self.sensors[name]
        value = sensor.read()
        logging.info(sensor.verbose(value))
        value['device'] = self.device_name
        value['path'] = '/sensors/{}'.format(name)

        topic = self.make_sensor_topic(name)
        self.mqttc.publish(topic, json.dumps(value))

    async def _add_polling_loop(self, sensor: str, delay: float):
        while True:
            self.publish_sensor_data(sensor)
            await asyncio.sleep(delay)
