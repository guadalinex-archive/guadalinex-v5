# -*- coding: utf-8 -*-
# Copyright (C) 2006-2007  Vodafone España, S.A.
# Author:  Pablo Martí
#
# This program is free software; you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation; either version 2 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License along
# with this program; if not, write to the Free Software Foundation, Inc.,
# 51 Franklin Street, Fifth Floor, Boston, MA 02110-1301 USA.

__version__ = "$Rev: 1172 $"

import os

import codecs
import gettext

gettext.bindtextdomain('VMC', os.getenv('TEXTDOMAINDIR', '/usr/share/locale'))
gettext.textdomain('VMC')
_ = gettext.gettext

ucs2_encoder = codecs.getencoder("utf_16be")
ucs2_decoder = codecs.getdecoder("utf_16be")
hex_decoder = codecs.getdecoder("hex_codec")

def int2hexstr(i):
    """
    Returns the hex representation of C{i}
    
    @rtype: str
    """
    return '%02X' % i

def swap(source):
    """
    Convert a string of numbers into the semi-octet representation
    described in GSM 03.40, 9.1.2.3
    """
    out = ''
    if len(source) % 2:
        source += 'F'

    for i in range(0, len(source), 2):
        out += source[i + 1]
        out += source[i]

    return out

def pack_7bit_bytes(s):
    """
    pack_7bit_bytes("hellohello") => "E8329BFD4697D9EC37"

    Packs a series of 7-bit bytes into 8-bit bytes and then to
    ASCII strings as hexidecimal. See GSM 03.40, 9.1.2.1
    """
    out = ''
    for i in range(0, len(s)):
        if i % 8 == 7:
            continue
        
        end = ord(s[i]) >> (i % 8)

        if i + 1 < len(s):
            start = ord(s[i + 1]) << (7 - (i % 8))
            start = start & 0xFF
            out += int2hexstr(start | end)
        else:
            out += int2hexstr(end)

    return out

def unpack_7bit_bytes(octets):
    """Decode 7 bit character strings packed into 8 bit byte strings"""
    septets = []
    overflow = overflow_len = 0

    for value in octets:
        septets.append(value << overflow_len & 127 | overflow)
        overflow = value >> (7 - overflow_len)
        overflow_len += 1
        if overflow_len == 7:
            septets.append(overflow)
            overflow = overflow_len = 0

    return septets

def pack_8bit_bytes(s):
    return "".join([int2hexstr(ord(c)) for c in s])

def pack_ucs2_bytes(s):
    return "".join([int2hexstr(ord(c)) for c in ucs2_encoder(s)[0]])

def unpack_ucs2_bytes(s):
    octets = [ord(c) for c in hex_decoder(s)[0]]
    user_data = "".join(chr(o) for o in octets)
    return ucs2_decoder(user_data)[0]

def check_if_ucs2(text):
    """Returns True if C{text} is a string encoded in UCS2"""
    if isinstance(text, str) and text.startswith('00'):
        try:
            unpack_ucs2_bytes(text)
        except (UnicodeDecodeError, TypeError):
            return False
        else:
            return True
    
    return False

def from_u(s):
    return isinstance(s, unicode) and s.encode('utf8') or s

def from_ucs2(s):
    return check_if_ucs2(s) and unpack_ucs2_bytes(s) or s
    
def to_u(s):
    """Returns a unicode object from C{s} if C{s} is not unicode already"""
    return isinstance(s, unicode) and s or unicode(s, 'utf8')
