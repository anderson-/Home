#!/usr/bin/env python

# Copyright (C) 2014 SISTEMAS O.R.P.
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
# along with this program.  If not, see <http://www.gnu.org/licenses/>.

import sys
import binascii
import struct
import select
import socket
import errno

MAX_TIMEOUT = 500
SUCCESS = "success"
FAILED = "failed"
PORT = 50000
#-------------------------------------------------------------------------------------------------
'''
Class containing the data from non-contiguous memory allocations
'''
class Data:
	def __init__(self, begin, data):
		self.begin = begin
		self.data = data
		self.count = len(data)
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	line: The line to parse
Returns:
	The size of data. The address of data. The type of data. The line checksum. True if the checksum is correct, otherwise False.
Description:
	It parses a line from the .hex file.
'''
def parse_line(line):
		ok = False
		size = int(line[1:3], 16)
		address = int(line[3:7], 16)
		type = int(line[7:9], 16)
		next_index = (9 + size * 2)
		data = binascii.a2b_hex(line[9:next_index])
		checksum = int(line[next_index:], 16)

		#checking if checksum is correct
		sum = size + (address >> 8) + (address & 0xFF) + type
		for byte in data:
			sum += ord(byte)

		if (~(sum & 0xFF) + 1) & 0xFF == checksum:
			ok = True

		return (size, address, type, data, checksum, ok)
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	chunks: An array with different chunks of data.
	path: The path to the .hex file to read
Returns:
	True if the reading was successfully, otherwise False.
Description:
	It reads a .hex file and stores the data in memory.
'''
def read_hex_file(chunks, path):
	try:
		file = open(path, 'r')
	except IOError:
		print "Hex file not loaded"
		return False
	line = file.readline()
	if line[0] != ':':
		print "The file seems to be a not valid .hex file"
		file.close()
		return False

	size, address, type, data, checksum, ok = parse_line(line.strip())
	if not ok:
		print "The checksum in line 1 is wrong"
		file.close()
		return False

	chunks.append(Data(address, data))

	# Read the other lines
	index = 0
	count = 2
	for line in file:
		size, address, type, data, checksum, ok = parse_line(line.strip())
		if not ok:
			print "The checksum in line", count, "is wrong"
			file.close()
			return False

		if chunks[index].begin + chunks[index].count == address:
			chunks[index].count += size
			for code in data:
				chunks[index].data += code
		else:
			chunks.append(Data(address, data))
			index += 1
		count += 1

	return True
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	None
Returns:
	The server socket
Description:
	It opens a server socket at the specified port and listens to connections.
'''
def init_server():
	server = socket.socket(socket.AF_INET, socket.SOCK_STREAM)
	server.bind(('', PORT))
	server.listen(1)
	return server
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	cli: The client socket
	response: The search string
	timeout: The maximum time in milliseconds the function can be running before a time out.
Returns:
	True if the string was found, otherwise False. The received string.
Description:
	It waits for the expected string.
'''
def wait_for(cli, response, timeout):
	inputs = [cli]
	received = ""
	milliseconds = 0
	while milliseconds < timeout:
		rlist, wlist, xlist = select.select(inputs,[],[], 0.001)
		if len(rlist) > 0:
			received += cli.recv(1)
			if response in received:
				return True, received
		milliseconds += 1

	return False, received
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	cli: The client socket
	timeout: The maximum time in milliseconds the function can be running before a time out.
	length: The number of bytes to receive.
Returns:
	True if the string has the required length, otherwise False. The received string.
Description:
	It waits for the required length of bytes.
'''
def return_data(cli, timeout, length = 1):
	inputs = [cli]
	received = ""
	milliseconds = 0
	while milliseconds < timeout:
		rlist, wlist, xlist = select.select(inputs,[],[], 0.001)
		if len(rlist) > 0:
			received = cli.recv(length)
			return True, received
		milliseconds += 1

	return False, received
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	cli: The client socket
Returns:
	True if the string was found, otherwise False
Description:
	It waits for the acknowledge string.
'''
def acknowledge(cli):
	if wait_for(cli, "\x14\x10", MAX_TIMEOUT)[0]: #STK_INSYNC, STK_OK
		print SUCCESS
		return True
	else:
		print FAILED
		return False
#-------------------------------------------------------------------------------------------------
'''
Parameters:
	chunks: An array with different chunks of data.
	cli: The client socket
Returns:
	Nothing
Description:
	It starts the STK500 protocol to program the data at their respective memory address.
'''
def program_process(chunks, cli):
	print "Connection to Arduino bootloader:",

	counter = 0
	cli.send("\x30\x20") #STK_GET_SYNCH, SYNC_CRC_EOP
	if not acknowledge(cli):
		return

	print "Enter in programming mode:",
	cli.send("\x50\x20") #STK_ENTER_PROGMODE, SYNC_CRC_EOP
	if not acknowledge(cli):
		return

	print "Read device signature:",
	cli.send("\x75\x20") #STK_READ_SIGN, SYNC_CRC_EOP
	if wait_for(cli, "\x14", MAX_TIMEOUT)[0]: #STK_INSYNC
		ok,received = return_data(cli, MAX_TIMEOUT, 3)
		print binascii.b2a_hex(received)
		if not wait_for(cli, "\x10", MAX_TIMEOUT)[0]: #STK_INSYNC
			print FAILED
			return
	else:
		print FAILED
		return

	for chunk in chunks:
		total = chunk.count
		if total > 0: #avoid the last block (the last line of .hex file)
			current_page = chunk.begin
			pages = total / 0x80
			index = 0

			for page in range(pages):
				print "Load memory address",current_page,":",
				cli.send(struct.pack("<BHB", 0x55, current_page, 0x20)) #STK_LOAD_ADDRESS, address, SYNC_CRC_EOP
				if not acknowledge(cli):
					return

				print "Program memory address:",
				cli.send("\x64\x00\x80\x46" + chunk.data[index:index + 0x80] + "\x20") #STK_PROGRAM_PAGE, page size, flash memory, data, SYNC_CRC_EOP
				if not acknowledge(cli):
					return
				current_page += 0x40
				total -= 0x80
				index += 0x80

			if total > 0:
				print "Load memory address",current_page,":",
				cli.send(struct.pack("<BHB", 0x55, current_page, 0x20)) #STK_LOAD_ADDRESS, address, SYNC_CRC_EOP
				if not acknowledge(cli):
					return

				print "Program memory address:",
				cli.send(struct.pack(">BHB", 0x64, total, 0x20) + chunk.data[index:index + total] + "\x20") #STK_PROGRAM_PAGE, page size, flash memory, data, SYNC_CRC_EOP
				if not acknowledge(cli):
					return

	print "Leave programming mode:",
	cli.send("\x51\x20") #STK_LEAVE_PROGMODE, SYNC_CRC_EOP
	acknowledge(cli)
#-------------------------------------------------------------------------------------------------
def main():
	print "Arduino remote programmer 2014 (c) SISTEMAS O.R.P"

	print "Listen to connections"
	ser = init_server()
	inputs = [ser]

	while True:
		rlist, wlist, xlist = select.select(inputs,[],[])
		for s in rlist:
			if s == ser:
				cli, addr = s.accept()
				print addr[0], "connected"
				# It assures the connection is for programming an Arduino and not other service.
				if wait_for(cli, "hello", 5000)[0]:
					cli.send("welcome")
					ok, received = wait_for(cli, "hex", 5000)
					if ok:
						chunks = []
						print "Read hex file", received.strip()
						if read_hex_file(chunks, received.strip()):
							cli.send("ok")
							# Wait for the byte '0' sent by Arduino after resetting
							if wait_for(cli, "\x00", MAX_TIMEOUT)[0]:
								program_process(chunks, cli)
						else:
							cli.send("error")
				cli.close()
				print "Listen to connections"
#-------------------------------------------------------------------------------------------------
if __name__ == "__main__":
	main()
