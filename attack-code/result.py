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

import re

class Result:

    regex = re.compile(
        '\('
            '([0-9]+), '    # waits
            '([0-9]+), '    # vid
            '([0-9]+), '    # delay
            '([0-9]+)'      # duration
        '\) => '
        '(running|broken|glitch|success)'  # result
        '\n'
    )

    def from_line(line, to_message=None):
        match = Result.regex.match(line)
        if match:
            return Result(line, match)
        return None

    def __init__(self, line, match):
        self.line = line
        self.waits = int(match[1])
        self.vid = int(match[2])
        self.delay = int(match[3])
        self.duration = int(match[4])
        self.result = match[5]

def read_from_file(filename, warnings=True):
    with open(filename, 'r') as f:
        for line in f:
            result = Result.from_line(line)
            if not result:
                if warnings:
                    print(f'Warning: Couldn\'t parse line "{line}"!')
                continue
            yield result

def results_by_type(results):
    result_dict = {
        'running' : list(),
        'broken' : list(),
        'success' : list(),
    }
    for res in results:
        result_dict[res.result].append(res)
    return result_dict

