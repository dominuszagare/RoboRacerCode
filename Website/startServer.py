from flask import Flask, jsonify, render_template, request, Response
import serial
import threading
import time
import struct

import cv2


app = Flask(__name__,
            static_url_path='', 
            static_folder='static',
            template_folder='templates')


SERIAL_PORT = 'COM5'
SERIAL_RATE = 115200
runApp = True

@app.route('/video_feed')
def video_feed():
    return Response(show_webcam(True), mimetype='multipart/x-mixed-replace; boundary=frame')

def show_webcam(mirror=False):
    global cameraImage
    cam = cv2.VideoCapture(1)
    while True:
        ret_val, cameraImage = cam.read()
        if not ret_val:
            break
        if mirror: 
            cameraImage = cv2.flip(cameraImage, 1)

        #cv2.imshow('my webcam', cameraImage)

        ret, buffer = cv2.imencode('.jpg',cameraImage)
        frame = buffer.tobytes()
        yield (b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')


        if cv2.waitKey(1) == 27: 
            break  # esc to quit
    cv2.destroyAllWindows()



floatVals = [0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]

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
        q3=floatVals[6],
        pozX=floatVals[7],
        pozY=floatVals[8],
        gX=floatVals[9],
        gY=floatVals[10],
        gZ=floatVals[11])
    #print("send", floatVals[11])
    return jsonData


@app.route('/')
def index():
    return render_template('index.html')

def readSerialData28():
    global floatVals, runApp

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

                sync = 0 
                    # 0 - looking for AB, 1 - looking for AA, 2 - found packet, 3 - packet done
                lenght = 12
                data = bytearray([0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0,0])
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
                            lenght = 0
                            continue
                            #print("sync!")

                    if sync == 4 and lenght < 48:
                        data[lenght] = int.from_bytes(byte_in, byteorder='big')
                        lenght += 1

                    if sync == 4 and lenght > 47:
                        sync = 5
                        #data = ser.read(28) #preberi preostalih 28 baytov
                        #print(data)
                        pitch = struct.unpack('<f',data[0:4])
                        roll = struct.unpack('<f',data[4:8])
                        yaw = struct.unpack('<f',data[8:12])
                        q0 = struct.unpack('<f',data[12:16])
                        q1 = struct.unpack('<f',data[16:20])
                        q2 = struct.unpack('<f',data[20:24])
                        q3 = struct.unpack('<f',data[24:28])

                        floatVals[7] = struct.unpack('<f',data[28:32])
                        floatVals[8] = struct.unpack('<f',data[32:36])
                        floatVals[9] = struct.unpack('<f',data[36:40])
                        floatVals[10] = struct.unpack('<f',data[40:44])
                        floatVals[11] = struct.unpack('<f',data[44:48])

                        floatVals[0] = pitch
                        floatVals[1] = roll
                        floatVals[2] = yaw
                        floatVals[3] = q0
                        floatVals[4] = q1
                        floatVals[5] = q2
                        floatVals[6] = q3
                        #print("serial",floatVals[11]) #odkomentiraj ce dobimo prave vrednosti
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
    
    #t1 = threading.Thread(target=readSerialData28, args=())
    #t1.start()

    #t2 = threading.Thread(target=show_webcam, args=())
    #t2.start()
    
    app.run(debug=True, port=6555, host='0.0.0.0')
    runApp = False
    print("DONE")
    