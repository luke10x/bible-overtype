#!/usr/bin/env python3
# https://en.wikipedia.org/wiki/Chapters_and_verses_of_the_Bible#Chapters

import subprocess
import os
import re

books = [
    ["Genesis", "Gen", "Ge", "Gn" ],
    ["Exodus", "Ex", "Exod" ],
    ["Leviticus", "Lev", "Le", "Lv" ],
    ["Numbers", "Num", "Nu", "Nm", "Nb" ],
    ["Deuteronomy", "Deut", "De", "Dt" ],
    ["Joshua", "Josh", "Jos", "Jsh." ],
    ["Judges", "Judg", "Jdg", "Jg", "Jdgs" ],
    ["Ruth", "Ruth", "Rth", "Ru" ],
    ["1 Samuel", "1 Sam", "1 Sm", "1 Sa", "1 S" ],
    ["2 Samuel", "1 Sam", "2 Sm", "2 Sa", "2 S" ],
    ["1 Kings", "1 Kgs", "1 Kin", "1 Ki" "1 K" ],
    ["2 Kings", "2 Kgs", "2 Kin", "2 KiK", "2 K" ],
    ["1 Chronicles", "1 Chr", "1 Ch", "1 Chron" ],
    ["2 Chronicles", "2 Chr", "2 Ch", "2 Chron" ],
    ["Ezra", "Ezr", "Ez" ],
    ["Nehemiah", "Neh", "Ne" ],
    ["Esther", "Esth", "Est", "Es" ],
    ["Job", "Jb" ],
    ["Psalms", "Ps", "Psalm", "Pslm", "Psa", "Psm" ],
    ["Proverbs", "Prov", "Pro", "Prv", "Pr" ],
    ["Ecclesiastes", "Ecc1", "Eccles", "Eccle", "Ecc", "Ec" ],
    ["Song of Solomon", "Solomon", "Song", "Canticles", "Cant" ],	
    ["Isaiah", "Isa", "Is" ],
    ["Jeremiah", "Jer", "Je", "Jr" ],
    ["Lamentations", "Lam", "La" ],
    ["Ezekial", "Ezek", "Eze", "Ezk" ],
    ["Daniel", "Dan", "Da", "Dn" ],
    ["Hosea", "Hos", "Ho" ],
    ["Joel", "Jl" ],
    ["Amos", "Am" ],
    ["Obadiah", "Ob" ],
    ["Jonah", "Jon", "Jnh" ],
    ["Micah", "Mic", "Mc" ],
    ["Nahum", "Nah", "Na" ],
    ["Habakkuk", "Hab" ],
    ["Zephaniah", "Zeph", "Zep", "Zp" ],
    ["Haggai", "Hag", "Hg.", "" ],
    ["Zechariah", "Zech", "Zec", "Zc" ],
    ["Malachi", "Mal", "Ml" ],
    ["Matthew", "Mt", "Matt" ],
    ["Mark", "Mk", "Mrk" ],
    ["Luke", "Lk", "Luk" ],
    ["John", "Jn", "Jhn" ],
    ["Acts of the Apostles", "Acts" ],
    ["Romans", "Rom", "Ro", "Rm" ],
    ["1 Corinthians", "1 Cor", "1 Co" ],
    ["2 Corinthians", "2 Cor", "2 Co" ],
    ["Galatians", "Gal", "Ga" ],
    ["Ephesians", "Eph", "Ephes" ],
    ["Philippians", "Phil", "Php", "Pp" ],
    ["Colossians", "Col" ],
    ["1 Thessalonians", "1 Thess", "1 Thes", "1 Th" ],
    ["2 Thessalonians", "2 Thess", "2 Thes", "2 Th" ],
    ["1 Timothy", "1 Tim", "1 Ti" ],
    ["2 Timothy", "2 Tim", "2 Ti" ],
    ["Titus", "Titus", "Tit", "Ti" ],
    ["Philemon", "Philem", "Phm", "Pm" ],
    ["Hebrews", "Heb"],
    ["James", "Jas", "Jm" ],
    ["1 Peter", "1 Pet", "1 Pe", "1 Pt", "1P" ],
    ["2 Peter", "2 Pet", "Pe", "2 Pt", "2P" ],
    ["1 John", "1 Jn", "1 Jhn", "1 J" ],
    ["2 John", "2 Jn", "2 Jhn", "2 J" ],
    ["3 John", "3 Jn", "3 Jhn", "3 J" ],
    ["Jude", "Jud", "Jd" ],
    ["Revelation", "Rev" ],
]

filename = "resources/the-king-james-bible.txt"

SCANNING = 1
BUILDING_CHAPTER = 2
BUILDING_VERSE = 4
BUILDING_REST = 5
FINISHED = 6

def is_number(byte):
    return b'0' <= byte <= b'9'

def is_colon(byte):
    return byte == b':'

def is_new_line(byte):
    return byte == b'\n'

def int2byte(intik):
    return intik.to_bytes(1, byteorder='big')

class Line:
    def __init__(self):
        self.state = SCANNING
        self.buf = b''
        self.next = None
    def accept(self, i, byte):
        if self.state == SCANNING:
            if (is_number(byte)):
                self.state = BUILDING_CHAPTER
                self.start = i 
        if self.state == BUILDING_CHAPTER:
            if (is_number(byte)):
                self.buf = self.buf + byte
            elif is_colon(byte):
                self.chapter = self.buf
                self.buf = self.buf + byte
                self.state = BUILDING_VERSE
                return
            else:
                self.drop()
        if self.state == BUILDING_VERSE:
            if (is_number(byte)):
                self.buf = self.buf + byte
            else:
                last = (self.buf[-1]).to_bytes(1, byteorder='big')
                if is_number(last):
                    self.verse = self.buf
                    self.state = BUILDING_REST
                else:
                    self.drop
        if self.state == BUILDING_REST:
            before_last = (self.buf[-2]).to_bytes(1, byteorder='big')
            last = (self.buf[-1]).to_bytes(1, byteorder='big')

            if is_number(byte):
                if self.next is None:
                    self.next = Line()

            if self.next is not None:
                self.next.accept(i, byte)
                if self.next.is_valid():
                    self.end = self.next.start - 1
                    # excess_chars = len(self.next.buf)

                    excess_chars = self.start + len(self.buf) - self.end
                    self.buf = self.buf[:-excess_chars]
                    self.state = FINISHED

            if is_new_line(byte) and is_new_line(last) and is_new_line(before_last):                
                self.state = FINISHED
                self.end = i
            else:
                self.buf = self.buf + byte

    def drop(self):
        self.state = SCANNING
        self.buf = b''

    def is_finished(self):
        return self.state == FINISHED
    
    def is_valid(self):
        return self.state == BUILDING_REST

def from_file_byte_yielder(filename):
    with open(filename, "rb") as f:
        while 1: 
            byte = f.read(1)
            if byte == b"":
                yield b""
            yield byte

def line_yielder(byte_yielder):
    i = 0
    line = Line()
    cursor = 0
    gapbuf = b''

    for byte in byte_yielder:
        line.accept(i, byte)
        gapbuf = gapbuf + byte
        if line.is_finished():    
            
            if line.start - 1 > cursor:
                gap = Line()
                gap.start = cursor
                gap.end = line.start - 1
                gap.verse = b"XXXXXX"
                excess_chars = gap.start + len(gap.buf) - gap.end
                gap.buf = gapbuf[:-excess_chars]
                yield gap
            
            cursor = line.end
            gapbuf = b''

            yield line

            next = line.next
            if next is None:
                line = Line()
            else:
                line = next
            
        if byte == b"":
            return
        i = i + 1

i = 0
result = {}
book = {"names": [], "chapters": [] }

i = 0

name = books[0][0]
for l in line_yielder(from_file_byte_yielder(filename)):
    i = i + 1
    # if i > 15000:
    #     break
    if l.buf == b'\n':
        continue
    if l.verse == b'XXXXXX':
        names = books.pop(0)

        name = names[0]
        names.append(l.buf.strip().decode())
        # print(f"{l.verse} {l.start}-{l.end} {names}")
        result[name] = {}
        result[name]["names"] = names
        result[name]["chapters"] = {}
    else:
        if not hasattr(l, 'chapter'):
            print(f"big prob: {l.buf}" )
        if int(l.chapter) not in result[name]["chapters"]:
            result[name]["chapters"][int(l.chapter)] = {}
            result[name]["chapters"][int(l.chapter)]["start"] = l.start
        result[name]["chapters"][int(l.chapter)]["end"] = l.end
        
# import json
# print(json.dumps(result, sort_keys=False, indent=4))

with open('chapter-index-kjv.bin', 'wb') as file:
    # print("books:")
    for b in result:
        # print(f"- name: {b}")
        # print(f"  aliases: {result[b]['names']}")
        # print(f"  chapters: {result[b]['chapters']}")
        for c in result[b]['chapters']:
            # print(f"  each: {result[b]['chapters'][c]}")
            d = result[b]['chapters'][c]
            print(d['start'], d['end'])
            file.write((d['start']).to_bytes(4, byteorder='big', signed=False))
            file.write((d['end']).to_bytes(4, byteorder='big', signed=False))