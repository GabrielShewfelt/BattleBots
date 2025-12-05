import socket,json

import RPi.GPIO as GPIO
import time
import threading
GPIO.setmode(GPIO.BOARD)
GPIO.setup(7,GPIO.OUT)#drive back
GPIO.setup(11,GPIO.OUT)#drive forward
GPIO.setup(13,GPIO.OUT)#arm down
GPIO.setup(15,GPIO.OUT)#arm up
GPIO.setup(16,GPIO.IN,pull_up_down=GPIO.PUD_UP)#button

GPIO.output(7,False)
GPIO.output(11,False)
GPIO.output(13,False)
GPIO.output(15,False)

DISCOVERY_PORT = 5006
PLAYER_NUMBER = 1

sock = socket.socket(socket.AF_INET, socket.SOCK_DGRAM)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_REUSEADDR, 1)
sock.setsockopt(socket.SOL_SOCKET, socket.SO_BROADCAST, 1)

sock.bind(("0.0.0.0", DISCOVERY_PORT))

returnAddress = "0"

def buttonCheck():
	pressed = False
	while True:
		if GPIO.input(16) == GPIO.LOW:
			if not pressed:
				print("PRESSED")
				if returnAddress != "0":
					print("Sending to " + str(returnAddress))
					sock.sendto(b"hit", returnAddress)
			pressed = True
			time.sleep(0.3)
		else:
			pressed = False
		time.sleep(0.01)
		
t = threading.Thread(target=buttonCheck)
t.start()

while True:
	print("Waiting")
	data, addr = sock.recvfrom(1024)
	if data == b"finding_rpis":
		returnAddress = addr
		sock.sendto(b"rpi", addr)
		print("Replied to find from", addr)
	elif data == b"armup":
		GPIO.output(13,False)
		GPIO.output(15,True)
	elif data == b"armdown":
		GPIO.output(13,True)
		GPIO.output(15,False)
	elif data == b"armstop":
		GPIO.output(13,False)
		GPIO.output(15,False)
	elif data == b"back":
		GPIO.output(7,True)
		GPIO.output(11,False)
	elif data == b"forward":
		GPIO.output(7,False)
		GPIO.output(11,True)
	elif data == b"stop":
		GPIO.output(7,False)
		GPIO.output(11,False)
		

