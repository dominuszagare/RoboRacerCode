from flask import Flask, jsonify, render_template, request, Response
import serial
import threading
import time
import struct
import random
import numpy as np


import main as arm
import cv2.aruco as aruco
import cv2
import os
import glob
import yaml
import math

#from IntStiskanje import kodiranje


app = Flask(__name__,
            static_url_path='', 
            static_folder='static',
            template_folder='templates')


SERIAL_PORT = 'COM5'
SERIAL_RATE = 115200
runApp = True

MERI_RAZDALJO = True
podatki_senzorja = [0,0,0,0]
floatVals = [0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0,0.0]
pozicijaARtaga = [0]


VIDEO_FEED = True


@app.route('/_getData',methods = ['GET', 'POST'])

def getData():
    global floatVals, podatki_senzorja, pozicijaARtaga
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
        gZ=floatVals[11],
        distAr=floatVals[12],
        arPoz=pozicijaARtaga,
        podatki_senzorja=podatki_senzorja
        )
    #print("send", podatki_senzorja)
    return jsonData

@app.route('/video_feed')
def video_feed():
    if VIDEO_FEED == True:
        return Response(show_webcam(), mimetype='multipart/x-mixed-replace; boundary=frame')
    else:
        return ""

"""
def stream(mirror=False):
    global image

    while True:
        cameraImage = image
        if mirror: 
            cameraImage = cv2.flip(cameraImage, 1)

        #cv2.imshow('my webcam', cameraImage)

        ret, buffer = cv2.imencode('.jpg',cameraImage)
        frame = buffer.tobytes()
        yield (b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + frame + b'\r\n')
"""

def show_webcam():
    global floatVals, pozicijaARtaga
    
    camera = cv2.VideoCapture(1)
    #impImage = cv2.imread("new.jpg")
    mtx, dist = arm.calibrateCamera()

    while True:
        status, image = camera.read()

        detected_tag = arm.detectTag(mtx, dist, image)

        if len(detected_tag[0]) != 0:
            for frame, tag, rvec, tvec in zip(detected_tag[0], detected_tag[1], detected_tag[2], detected_tag[3]):
                # image = superimposeImage(image, impImage, frame, tag)
                if tag is not None:
                    for i in range(tag.size):
                        upper_left = frame[0][0][0], frame[0][0][1]
                        upper_right = frame[0][1][0], frame[0][1][1]
                        lower_right = frame[0][2][0], frame[0][2][1]
                        lower_left = frame[0][3][0], frame[0][3][1]

                        frame_pos = frame.reshape((4, 2))

                        (up_left, up_right, low_right, low_left) = frame_pos
                        up_left = (int(up_left[0]), int(up_left[1]))

                        # rr, thet = ra.rArea(corners)
                        #print(frame)
                        aruco.drawAxis(image, mtx, dist, rvec[0], tvec[0], 0.06)  # np.array([0.0, 0.0, 0.0])
                        cv2.putText(image,
                                    "%.1f cm" % ((tvec[:, 2] * 100) - 10),
                                    (up_left[0], up_left[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (244, 244, 244), 2)
                        # cv2.circle(image, (100, int(rr / 600)), 6, (200, 40, 230), -1)
                        R, _ = cv2.Rodrigues(rvec[0])
                        floatVals[12] =  ((tvec[:, 2] * 100) - 10)[0]
                        cameraPose = -R.T * tvec[0]
                        #print("XY",cameraPose)
                        poz_dx = int(low_right[0]) - int(up_left[0])
                        poz_dy = int(low_right[1]) - int(up_left[1])
                        pozicijaARtaga = [((tvec[:, 2] * 100) - 10)[0],int(up_left[0])+int(poz_dx/2), int(up_left[1])+int(poz_dy/2)]
           


                        #print("Distance:", pozicijaARtaga[0])
        else:
            pozicijaARtaga = [0]

        ret, Imagebuffer = cv2.imencode('.jpg',image)
        Imageframe = Imagebuffer.tobytes()
        yield (b'--frame\r\n'b'Content-Type: image/jpeg\r\n\r\n' + Imageframe + b'\r\n')
        cv2.waitKey(1)

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

def meriRazdaljo():
    global podatki_senzorja
    while True:
        #lista=[]
        #for x in range(10):
        #    r1 = random.randint(420, 435)
        #    lista.append(r1)
        
        podatki_senzorja = [2000,2000]
        #print(string)
        time.sleep(1)

if __name__ == '__main__':
    
    t1 = threading.Thread(target=readSerialData28, args=())
    t1.start()

    #t2 = threading.Thread(target=show_webcam, args=())
    #2.start()

    if MERI_RAZDALJO == True:
        t4 = threading.Thread(target=meriRazdaljo, args=())
        t4.start()
    
    app.run(debug=True, port=6555, host='0.0.0.0')
    runApp = False
    print("DONE")
    


