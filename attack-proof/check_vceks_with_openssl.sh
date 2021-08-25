#!/bin/bash
# Copyright (C) 2021 Niklas Jacob, Robert Buhren
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

tmp_pub_key=$(mktemp)

for sig_file in $(ls vcek_signatures)
do
    echo -n "Checking vcek_signatures/$sig_file: "
    hex_str=$(echo "$sig_file" | grep -oE "[0-f]{8}")
    openssl x509 -inform DER -in "certs/vcek/$hex_str.cert" -noout -pubkey -out "$tmp_pub_key" || exit
    openssl dgst -verify "$tmp_pub_key" -sha384 -signature "vcek_signatures/$sig_file" title.txt || exit
done

rm $(tmp_pub_key)
