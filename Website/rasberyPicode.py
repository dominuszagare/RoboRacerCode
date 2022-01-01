from flask import Flask, jsonify, render_template, request
import serial
import threading
import time
import struct
import spidev
import RPi.GPIO as GPIO

app = Flask(__name__,
            static_url_path='', 
            static_folder='static',
            template_folder='templates')


SERIAL_PORT = '/dev/ttyACM0'
SERIAL_RATE = 115200
USE_USART = True


SPI_RATE = 1953000
GPIO.setmode(GPIO.BCM)  # Sets the GPIO pin labelling mode
CS_PIN = 17
spi = spidev.SpiDev()
    
runApp = True

floatVals = [0.0,0.0,0.0,0.0,0.0,0.0,0.0]

@app.route('/_getData',methods = ['GET', 'POST'])
def getData():
    global floatVals
    jsonData = jsonify(
        pitch=floatVals[0],
        roll=floatVals[1],
        yaw=floatVals[2],
        q0=floatVals[3],
        q1=floatVals[4],
        q2=floatVals[5],
        q3=floatVals[6])
    #print("send", floatVals[0])
    return jsonData

@app.route('/')
def index():
    return render_template('index.html')
    
    
def write(data):
    """Write data to the device"""
    print(f'-- Writing to device')
    print(f'   sending "{data}"')
    rw_byte = 0x01
    nbytes = len(data)
    command_packet = bytes([rw_byte, nbytes])
    response = _spi_xfer(command_packet)
    data_packet = data.encode('utf-8')
    _spi_xfer(data_packet)

def read(nbytes):
    """ perform a read from the device"""
    print('-- Reading from device')
    rw_byte = 0x00
     
    command_packet = bytes([rw_byte, nbytes])
    response = _spi_xfer(command_packet)
    print(f'   we want to recive {nbytes} bytes')
    
    data_packet = bytes(nbytes)  # initialises a byte array of all zeros, with length = nbytes
    response = _spi_xfer(data_packet)
    print(f'   response = "{response}"')
    
def _spi_xfer(to_write):
    # assert CS
    GPIO.output(CS_PIN, 0)
    # do the actual transfer
    response = bytes(spi.xfer2(to_write))
    # release CS
    GPIO.output(CS_PIN, 1)
    
    return response

def TlakSpi():
    
    GPIO.setup(CS_PIN, GPIO.OUT)
    GPIO.output(CS_PIN, 1)  # set CS initially to high. CS is pulled low to start a transfer
    
    spi.open(0, 0)
    spi.max_speed_hz = SPI_RATE
    spi.mode = 0
    
    #spi.xfer([msb, lsb])
    num = 8
    while True:
        
        num += 1
        if(num > 12):
            num = 8
            
        read(num)
        time.sleep(1)
    

def readSerialData28():
    global floatVals

    serialWorks = False
    while runApp:
        ser = []
        if serialWorks == False:
            try:
                print("Opening port:", SERIAL_PORT)
                ser = serial.Serial(SERIAL_PORT, SERIAL_RATE)
                serialWorks = True

            except serial.SerialException:
                print("Could not open serial port!")
                serialWorks = False
                time.sleep(3)

        while serialWorks:
            try:
                #data = ser.read(30)
                #print(data)
                data = bytearray([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0])
                #data = []
                sync = 0 
                   # 0 - looking for AB, 1 - looking for AA, 2 - found packet, 3 - packet done
                lenght = 12
                while sync < 5:
                    byte_in = ser.read()
                    #print(byte_in)
                    if sync == 0:
                       if byte_in == b'\xAB': #poravnava na podatkovno vodilo ce kombiniras 16bitn tip z 32bitnem v strukturi na 32bitnem sistemu potem 16bitni tip zasede 32bitov
                           sync = 1
                           continue
                    if sync == 1:
                       if byte_in == b'\xAA':
                           sync = 2
                           continue
                    if sync == 2:
                       if byte_in == b'\x00':
                           sync = 3
                           continue
                           
                    if sync == 3:
                        if byte_in == b'\x00':
                            #print("sync!")
                            sync = 4
                            lenght = 0
                            #data=[]
                            continue

                    if sync == 4 and lenght < 28:
                        #data.append(byte_in)
                        data[lenght] = int.from_bytes(byte_in, byteorder='big')
                        lenght += 1

                    if lenght > 27 and sync == 4:
                        sync = 5
                        #data = ser.read(28) #preberi preostalih 28 baytov
                        #data = bytearray(data)
                        #print(data)
                        pitch = struct.unpack('<f',data[0:4])
                        roll = struct.unpack('<f',data[4:8])
                        yaw = struct.unpack('<f',data[8:12])
                        q0 = struct.unpack('<f',data[12:16])
                        q1 = struct.unpack('<f',data[16:20])
                        q2 = struct.unpack('<f',data[20:24])
                        q3 = struct.unpack('<f',data[24:28])
                        floatVals[0] = pitch
                        floatVals[1] = roll
                        floatVals[2] = yaw
                        floatVals[3] = q0
                        floatVals[4] = q1
                        floatVals[5] = q2
                        floatVals[6] = q3
                        #print("serial",floatVals[0])
                        #print(pitch,roll,yaw,q0,q1,q2,q3)
                            
            except serial.SerialException:
                print("Closing serial",serial.SerialException)
                ser.close()
                serialWorks = False
                time.sleep(1)
            
    try:
        ser.close()
    except:
        print("serial is not open")

if __name__ == '__main__':
    
    if USE_USART:
        t1 = threading.Thread(target=readSerialData28, args=())
        t1.start()
    else:
        t1 = threading.Thread(target=TlakSpi, args=())
        t1.start()
    
    app.run(debug=True, port=6555, host='10.3.141.1')
    runApp = False
    print("DONE")
    
