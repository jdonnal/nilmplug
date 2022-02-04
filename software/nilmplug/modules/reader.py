import asyncio
import os
import random

import joule.api
import numpy as np

from joule.client.reader_module import ReaderModule
from joule.errors import ApiError
from nilmplug.models.plug import Plug

CSS_DIR = os.path.join(os.path.dirname(__file__), 'assets', 'css')
JS_DIR = os.path.join(os.path.dirname(__file__), 'assets', 'js')
TEMPLATES_DIR = os.path.join(os.path.dirname(__file__), 'assets', 'templates')

ARGS_DESC = """
TODO
"""


class PlugReader(ReaderModule):  # pragma: no cover

    async def setup(self, parsed_args, app, output):
        self.plug = Plug(parsed_args.plug_ip)
        try:
            parts = parsed_args.event_stream.split('/')
            name = parts[-1]
            folder = '/'.join(parts[:-1])
            self.event_stream = joule.api.EventStream(name)
            self.event_stream = await self.node.event_stream_create(self.event_stream, folder)
        except ApiError:
            self.event_stream = await self.node.event_stream_get(parsed_args.event_stream)

    def custom_args(self, parser):
        parser.add_argument("--load-name", help="name of load", default="load")
        parser.add_argument("--plug-serial", help="plug serial number")
        parser.add_argument("--plug-ip", help="plug ip address")
        parser.add_argument("--event-stream", help="Joule event stream for on/off events")

    async def run(self, parsed_args, output):
        load_name = parsed_args.load_name
        last_ts = joule.utilities.time_now()
        event = None
        avg_power_acc = 0
        avg_power_count = 0
        start_kwh = None
        await self.plug.set_rtc()
        while not self.stop_requested:
            data = await self.plug.get_data(last_ts)
            if data is not None:
                for row in data:
                    ts = row[0]
                    avg_power = row[4]
                    kwh = row[7]
                    if avg_power > 5:  # load is on
                        if event is None:  # load just turned on
                            event = joule.api.Event(ts, end_time=ts)
                            start_kwh = kwh
                        avg_power_count += 1
                        avg_power_acc += avg_power
                    if avg_power < 5 and event is not None:  # load turned off
                        total_kwh = kwh - start_kwh
                        avg_power = avg_power_acc / avg_power_count
                        event.content = {
                            'average_power': avg_power,
                            'kwh': total_kwh,
                            'name': load_name
                        }
                        event.end_time = ts
                        await self.node.event_stream_write(self.event_stream, [event])
                        event = None
                        avg_power_acc = 0
                        avg_power_count = 0
                        start_kwh = None

                await output.write(data)
                last_ts = data[-1][0]
            await asyncio.sleep(20)


def main():  # pragma: no cover
    r = PlugReader()
    r.start()


if __name__ == "__main__":
    main()
