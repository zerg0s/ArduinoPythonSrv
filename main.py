# https://tutorialedge.net/python/concurrency/asyncio-event-loops-tutorial/
import os, sys
import asyncio
import platform

from datetime import datetime
from typing import Callable, Any

from aioconsole import ainput
from bleak import BleakClient, discover

OurArduino33Ble = "MySensorForTests"


class Connection:
    client: BleakClient = None

    def __init__(
            self,
            loop: asyncio.AbstractEventLoop,
            read_characteristic: str,
            write_characteristic: str,
            data_dump_handler: Callable[[str], None],
            data_dump_size: int = 256,
    ):
        self.loop = loop
        self.read_characteristic = read_characteristic
        self.write_characteristic = write_characteristic
        self.data_dump_handler = data_dump_handler

        self.last_packet_time = datetime.now()
        self.dump_size = data_dump_size
        self.connected = False
        self.connected_device = None
        self.rx_data = []
        self.rx_timestamps = []
        self.rx_delays = []

    def on_disconnect(self, client: BleakClient, future: asyncio.Future):
        self.connected = False
        # Put code here to handle what happens on disconnect.
        print(f"Disconnected from {self.connected_device.name}!")

    async def cleanup(self):
        if self.client:
            await self.client.stop_notify(read_characteristic)
            await self.client.disconnect()

    async def manager(self):
        print("Starting connection manager.")
        while True:
            if self.client:
                await self.connect()
            else:
                await self.select_device()
                await asyncio.sleep(5.0)

    async def connect(self):
        if self.connected:
            return
        try:
            await self.client.connect()
            self.connected = await self.client.is_connected()
            if self.connected:
                print(F"Connected to {self.connected_device.name}")
                self.client.set_disconnected_callback(self.on_disconnect)
                await self.client.start_notify(
                    self.read_characteristic, self.notification_handler,
                )
                while True:
                    if not self.connected:
                        break
                    await asyncio.sleep(1)
            else:
                print(f"Failed to connect to {self.connected_device.name}")
        except Exception as e:
            print(e)

    async def select_device(self):
        print("Bluetooh LE hardware warming up...")
        await asyncio.sleep(2.0)  # Wait for BLE to initialize.
        devices = await discover()
        devices = list(filter(lambda d: d.name != "Unknown", devices))
        deviceNames = list(map(lambda dev: dev.name, devices))
        if OurArduino33Ble in deviceNames:
            print(f"Found {OurArduino33Ble}")
            response = deviceNames.index(OurArduino33Ble)
        else:
            print("Please select device: ")
            for i, device in enumerate(devices):
                print(f"{i}: {device.name}")

            response = -1
            while True:
                response = await ainput("Select device: ")
                try:
                    response = int(response.strip())
                except:
                    print("Please make valid selection.")
                if response > -1 and response < len(devices):
                    break
                else:
                    print("Please make valid selection.")

        self.connected_device = devices[response]
        addressToConnect = devices[response].address
        print(f"Connecting..")
        self.client = BleakClient(addressToConnect)

    def record_time_info(self):
        present_time = datetime.now()
        self.rx_timestamps.append(present_time)
        self.rx_delays.append((present_time - self.last_packet_time).microseconds)
        self.last_packet_time = present_time

    def clear_lists(self):
        self.rx_data.clear()
        self.rx_delays.clear()
        self.rx_timestamps.clear()

    def notification_handler(self, sender: str, data: Any):
        self.rx_data.append(int.from_bytes(data, byteorder="big"))
        self.record_time_info()
        if len(self.rx_data) >= self.dump_size:
            self.data_dump_handler(str(self.rx_timestamps[-1].time()) + " " + data.decode("utf-8"))
            self.clear_lists()


#############
# Loops
#############
async def user_console_manager(connection: Connection):
    print("In Connection loop")



async def main():
    while True:
        # YOUR APP CODE WOULD GO HERE.
        await asyncio.sleep(0.5)


#############
# App Main
#############
read_characteristic = "00001143-0000-1000-8000-00805f9b34fb"
write_characteristic = "00001142-0000-1000-8000-00805f9b34fb"

if __name__ == "__main__":

    # Create the event loop.
    loop = asyncio.get_event_loop()

    connection = Connection(
        loop, read_characteristic, write_characteristic, print)

    try:
        asyncio.ensure_future(connection.manager())
        asyncio.ensure_future(user_console_manager(connection))
        asyncio.ensure_future(main())
        loop.run_forever()
    except KeyboardInterrupt:
        print()
        print("User stopped program.")
    finally:
        print("Disconnecting...")
        loop.run_until_complete(connection.cleanup())
