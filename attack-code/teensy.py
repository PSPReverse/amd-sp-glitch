# Copyright (C) 2021 Niklas Jacob
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <https://www.gnu.org/licenses/>.

import time
import re
import serial as pyserial

DEBUG = False

class TeensyClient:
    def __init__(self, device, baudrate=115200, timeout=2):

        self.device = device
        self.baudrate = baudrate
        self.timeout = timeout

        self.serial = None

        self.last_reset = None
        self.glitch_vid = None
        self.glitch_delay = None
        self.glitch_duration = None
        self.glitch_cooldown = None
        self.glitch_repeats = None
        self.attack_waits = None

    def connect(self):
        self.serial = None
        self.serial = pyserial.Serial(
            self.device,
            baudrate = self.baudrate,
            timeout = self.timeout
        )
        self.clear()

    def set_timeout(self, timeout):
        if self.timeout:
            if self.timeout == timeout:
                return
        self.timeout = timeout
        if self.serial:
            self.serial.timeout = timeout

    def cmd(self, cmd : str = None, timeout : int = 2) -> bytes:
        self.set_timeout(timeout)

        if cmd:
            self.serial.write(cmd.encode() + b'\r\n')

        prompt = b'\r\x1b[K> '
        res = self.serial.read_until(prompt)

        if DEBUG:
            print(f"{cmd} --> {res}")

        if cmd:
            if not res.startswith(cmd.encode() + b'\r\n'):
                print(f'Error: teensy didn\'t echo!\n"{cmd}" --> "{res}"')
                return None

            res = res[len(cmd)+2:]

        if res.endswith(prompt):
            res = res[:-len(prompt)]
        elif cmd:
            print(f'Error: didn\'t receive prompt!\n"{cmd}" --> "{res}"')
            return None

        if res.endswith(b'\r\n'):
            res = res[:-2]

        return res.decode('ascii', errors='backslachreplace')

    def cmd_expect(self, cmd : str, expected: str, **kwargs) -> bool:
        message = self.cmd(cmd, **kwargs)

        if message != expected:
            print(f'Error: "{message}" instead of "{expected}"!')
            return False

        return True

    def wait(self, **kwargs) -> str:
        return self.cmd(**kwargs)

    def wait_expect(self, expected: str, **kwargs) -> bool:
        message = self.wait(**kwargs)

        if message.startswith('\r\n'):
            message = message[2:]
        else:
            print('Warning: wait message didn\'t start with CRLF!')

        if message != expected:
            print(f'Error: "{message}" instead of "{expected}"!')
            return False

        return True

    def wait_match(self, expected, **kwargs):
        message = self.wait(**kwargs)

        if not message:
            print(f'Error: timeout instead of "{expected}"!')
            return None

        if message.startswith('\r\n'):
            message = message[2:]
        else:
            print('Warning: wait message didn\'t start with CRLF!')

        result = expected.match(message)

        if not result:
            print(f'Error: "{message}" instead of "{expected}"!')

        return result

    def clear(self) -> bool:
        for _ in range(10):
            try:
                self.serial.reset_input_buffer()
                time.sleep(.2)
                break
            except:
                pass

        time.sleep(.2)

        return self.cmd_expect('', '')

    def set(self, module : str, param : str, value : str, **kwargs) -> bool:
        return self.cmd_expect(f'set {module} {param} {value}', '', **kwargs)

    def wait_for_restart(self, **kwargs) -> bool:
        if self.wait_expect(
            'Restart detected!\r\n'
            'Setting VSoc!\r\n'
            'Setting VCore and disabling telemetry!',
            **kwargs
        ):
            self.last_reset = time.time()
            return True
        return False

    def reset_target(self, **kwargs) -> bool:
        if self.last_reset:
            while time.time() < self.last_reset + 3.0:
                time.sleep(.1)
        if not self.cmd_expect('restart reset', 'Resetting target!'):
            return False
        if not self.wait_expect('Target is now offline!'):
            return False
        return self.wait_for_restart(**kwargs)

    def arm_glitch(self, **kwargs) -> bool:
        return self.cmd_expect('glitch arm', 'Glitch armed!', **kwargs)

    def arm_attack(self, **kwargs) -> bool:
        return self.cmd_expect('attack', 'Attack armed!', **kwargs)

    __glitch_re = re.compile(
        'Glitch triggered!\r\n'
        'Target ([a-z ]*)!'
    )

    def wait_for_glitch(self, **kwargs) -> str:
        return self.wait_match(self.__glitch_re, **kwargs)

    __attack_re = re.compile(
        'Attack triggered!\r\n'
        'Target ([a-z ]*)!'
    )

    def wait_for_attack(self, **kwargs) -> str:
        return self.wait_match(self.__attack_re, **kwargs)

    def glitch(self,
        vid : int,
        delay : int,
        duration : int,
        cooldown : int = None,
        repeats : int = None,
        **kwargs
    ) -> str:

        if self.glitch_vid != vid:
            if not self.set('glitch', 'vid', vid, **kwargs):
                return None
            self.glitch_vid = vid

        if self.glitch_delay != delay:
            if not self.set('glitch', 'delay', delay, **kwargs):
                return None
            self.glitch_delay = delay

        if self.glitch_duration != duration:
            if not self.set('glitch', 'duration', duration, **kwargs):
                return None
            self.glitch_duration = duration

        if cooldown and self.glitch_cooldown != cooldown:
            if not self.set('glitch', 'cooldown', cooldown, **kwargs):
                return None
            self.glitch_cooldown = cooldown

        if repeats and self.glitch_repeats != repeats:
            if not self.set('glitch', 'repeats', repeats, **kwargs):
                return None
            self.glitch_repeats = repeats

        if not self.arm_glitch(**kwargs):
            return None

        match = self.wait_for_glitch(**kwargs)

        if not match:
            return None

        return {
            'continues running' : 'running',
            'glitched successfully' : 'glitch',
            'is broken' : 'broken',
        } [match[1]]

    def attack(self,
        waits : int,
        vid : int,
        delay : int,
        duration : int,
        cooldown : int = None,
        **kwargs
    ) -> str:

        if self.attack_waits != waits:
            if not self.set('attack', 'waits', waits, **kwargs):
                return None
            self.attack_waits = waits

        if self.glitch_vid != vid:
            if not self.set('glitch', 'vid', vid, **kwargs):
                return None
            self.glitch_vid = vid

        if self.glitch_delay != delay:
            if not self.set('glitch', 'delay', delay, **kwargs):
                return None
            self.glitch_delay = delay

        if self.glitch_duration != duration:
            if not self.set('glitch', 'duration', duration, **kwargs):
                return None
            self.glitch_duration = duration

        if cooldown and self.glitch_cooldown != cooldown:
            if not self.set('glitch', 'cooldown', cooldown, **kwargs):
                return None
            self.glitch_cooldown = cooldown

        if not self.arm_attack(**kwargs):
            return None

        self.reset_target(**kwargs)

        match = self.wait_for_attack(**kwargs)

        if not match:
            return None

        return {
            'continues running' : 'running',
            'glitched successfully' : 'success',
            'is broken' : 'broken',
        } [match[1]]

class GlitchSetup:
    def __init__(self, teensy, use_core = False, hw_cfg=1):

        self.use_core = use_core

        self.teensy = teensy
        self.hw_cfg = hw_cfg

        self.reuse_running_count = 0

    def start(self):
        self.teensy.connect()
        self.teensy.wait()
        print(self.teensy.cmd(f'set hw config {self.hw_cfg}'))
        if self.use_core:
            print(self.teensy.cmd(f'set glitch set_soc false'))
            print(self.teensy.cmd(f'set glitch set_core true'))
        self.teensy.clear()
        time.sleep(.2)

    def attack(self,
        waits : int,
        vid : int,
        delay : int,
        duration : int,
        **kwargs
    ) -> str:

        result = self.teensy.attack(waits, vid, delay, duration, **kwargs)

        if not result:
            return None

        if result not in ['running', 'success', 'broken']:
            print(f'Error: Unknown result "{result}"!')
            self.teensy.clear()
            return None

        return result

    def attack_range(self, count, waits, vid, delay_min, delay_max, dur_min, dur_max, exit_on_success=False, **kwargs):

        import numpy as np
        r = np.random.default_rng(int(time.time()))

        delays = r.integers(delay_min, delay_max, count)
        durations = r.integers(dur_min, dur_max, count)

        self.teensy.clear()
        for i in range(count):

            delay = delays[i]
            duration = durations[i]

            result = self.attack(waits, vid, delay, duration, **kwargs)

            if result:
                print(f'({waits}, {vid}, {delay}, {duration}) => {result}')
                if exit_on_success:
                    if result[0] == 'success':
                        return 'success'
