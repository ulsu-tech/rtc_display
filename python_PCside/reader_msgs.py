#!/usr/bin/env python3
import sys
import serial

serialPort = None

def main():
    portName = "ttyUSB0"
    if len(sys.argv) > 1:
        portName = sys.argv[1]

    serialPort = serial.Serial("/dev/" + portName)

    #serialPort.open()

    while True:
        v = serialPort.read(8)
        if len(v) < 8:
            print("Got not expected number of bytes (", len(v), ")")

        s = 0
        for a in v:
            s += a

        s %= 256
        wrValue = bytes([s])
        serialPort.write(wrValue)

        for a in v:
            print("%02x " % a,  end="")

        print("")

        # sequence is sec, min, hour, day, month, year, tem-temp
        print("%04d/" % ( int(v[5] /16) * 10 + v[5] % 16), end="")
        print("%02d/" % ( int(v[4]/16) * 10 +  v[4] % 16), end= "")
        print("%02d " % ( int(v[3]/16) * 10 +  v[3] % 16), end = "")

        print("%02d:" % ( int(v[2]/16) * 10 +  v[2] % 16), end = "")
        print("%02d:" % ( int(v[1]/16) * 10 +  v[1] % 16), end = "")
        print("%02d  " % ( int(v[0]/16) * 10 +  v[0] % 16), end = "")

        print("temp = ", (v[6] * 4 + (v[7]>>6)) * 0.5 - 273)

if __name__ == '__main__':
    main()
