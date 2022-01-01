from flask import Flask, jsonify, render_template, request
import serial
import threading
import time
import struct

app = Flask(__name__,
            static_url_path='', 
            static_folder='static',
            template_folder='templates')


SERIAL_PORT = 'COM5'
SERIAL_RATE = 115200
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

def readSerialData28():
    global floatVals

    serialWorks = True
    while runApp:
        ser = []
        
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
                            sync = 4
                            #lenght = 0
                            #print("sync!")
                    #if sync == 2 and lenght < 26:
                    #    data.append(byte_in)
                    #    lenght += 1

                    #if lenght > 25:
                    #    sync = 4
                    #    print(data)
                    #    time.sleep(4)
                    
                    if sync == 4:
                        sync = 5
                        data = ser.read(28) #preberi preostalih 28 baytov
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
                            
            except:
                print("Closing serial")
                ser.close()
                serialWorks = False
            
    try:
        ser.close()
    except:
        print("serial is not open")

if __name__ == '__main__':
    
    t1 = threading.Thread(target=readSerialData28, args=())
    t1.start()
    
    app.run(debug=True, port=6555, host='0.0.0.0')
    runApp = False
    print("DONE")
    