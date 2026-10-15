import gpiod
import time
from gpiod.line import Direction, Value
 
with gpiod.request_lines(
    '/dev/gpiochip9',
    consumer="blink",
    config={
        (15,): gpiod.LineSettings(direction=Direction.OUTPUT)
    }
) as request:
    try:
        while True:
            request.set_value(15, Value.ACTIVE)
            time.sleep(0.5)
            request.set_value(15, Value.INACTIVE)
            time.sleep(0.5)
    except KeyboardInterrupt:
        request.set_value(15, Value.INACTIVE)
