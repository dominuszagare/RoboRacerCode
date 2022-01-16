import numpy as np
import cv2
import cv2.aruco as aruco
import os
import glob
import yaml
import math
#https://automaticaddison.com/how-to-detect-aruco-markers-using-opencv-and-python/
#https://github.com/harshkakashaniya/AR-tag-detection
#https://medium.com/analytics-vidhya/real-time-distance-calculation-using-aruco-markers-b469d5f9791d
#https://www.pyimagesearch.com/2015/01/19/find-distance-camera-objectmarker-using-python-opencv/
#https://learnopencv.com/camera-calibration-using-opencv/
    #https://learnopencv.com/camera-calibration-using-opencv/
#https://stackoverflow.com/questions/56002672/display-an-image-over-another-image-at-a-particular-co-ordinates-in-opencv

def calibrateCamera():
    settings = (cv2.TERM_CRITERIA_EPS + cv2.TERM_CRITERIA_MAX_ITER, 30, 0.001)

    objp = np.zeros((49,3), np.float32)
    objp[:,:2] = np.mgrid[0:7,0:7].T.reshape(-1,2)

    worldPoints = [] # 3d pointi v svetu
    imagePoints = [] # 2d pointi v sliki

    calibrationImages = glob.glob(r'images/*.jpg')

    for imageList in calibrationImages:
        img = cv2.imread(imageList)
        #print(images[im_i])
        grayscale = cv2.cvtColor(img, cv2.COLOR_BGR2GRAY)

        # Corner detect
        ret, frames = cv2.findChessboardCorners(grayscale, (7,7), None)

        if ret == True: #če so najdeni cornerji šahovnice
            worldPoints.append(objp)

            chessCorner = cv2.cornerSubPix(grayscale,frames,(11,11),(-1,-1),settings)
            imagePoints.append(chessCorner)

            # Prikaz linij, za debuganje, če se narisani koti ne ujemajo z koti šahovnice
            img = cv2.drawChessboardCorners(img, (7,7), chessCorner, ret)

            cv2.imshow('img', img)
            cv2.waitKey(400)
    cv2.destroyAllWindows()
    # calibration data
    ret, mtx, dist, rvecs, tvecs = cv2.calibrateCamera(worldPoints, imagePoints, grayscale.shape[::-1], None, None)

    # zapiši kalibracijske podatke v list
    data = {'camera_matrix': np.asarray(mtx).tolist(),
            'dist_coeff': np.asarray(dist).tolist()}


    with open("matrix.yaml", "w") as f: # za nadaljno uporabo
        yaml.dump(data, f)

    return mtx,dist

def distance_to_camera(perWidth, knownWidth = 5, focalLength = 1296):

	return (knownWidth * focalLength) / perWidth


def detectTag(mtx, dist, image, tag_size=6, total_tags=250, paint=True):
    grayscale = cv2.cvtColor(image, cv2.COLOR_BGR2GRAY)
    token = getattr(aruco, f'DICT_{tag_size}X{tag_size}_{total_tags}')
    dic = aruco.Dictionary_get(token)
    arDetectParam = aruco.DetectorParameters_create()
    frame, tag, invalid_tag = aruco.detectMarkers(grayscale, dic, parameters=arDetectParam)

    aruco_size = 0.05
    rvec,tvec,_ = aruco.estimatePoseSingleMarkers(frame, aruco_size, mtx, dist)
    if paint:
        aruco.drawDetectedMarkers(image, frame)

    return [frame, tag, rvec,tvec]


def superimposeImage(image, impImage, frame, tag):
    y1 = frame[0][0][0], frame[0][0][1]
    x1 = frame[0][1][0], frame[0][1][1]
    x2 = frame[0][2][0], frame[0][2][1]
    y2 = frame[0][3][0], frame[0][3][1]

    x1 = (frame[0][0][0][0], frame[0][0][0][1])
    x2 = (frame[0][0][1][0], frame[0][0][1][1])
    x3 = (frame[0][0][2][0], frame[0][0][2][1])
    x4 = (frame[0][0][3][0], frame[0][0][3][1])

    height, width, mode = impImage.shape

    #imgPoint = np.array([ x1,  x2,  x3,  x4])
    shapeS = image.shape
    imgPoint_2 = np.float32([[0, 0], [width, 0], [width, height], [0, height]])
    getHomography, _ = cv2.findHomography(imgPoint_2, imgPoint)
    #pts_src = np.array(
     #   [
      #      [0, 0],
       #     [shapeS[1] - 1, 0],
        #    [shapeS[1] - 1, shapeS[0] - 1],
         #   [0, shapeS[0] - 1]
        #], dtype=float
    #);
    output = cv2.warpPerspective(impImage, getHomography, (image.shape[1], image.shape[0]))

    cv2.fillConvexPoly(image, imgPoint.astype(int), (0, 0, 0))
    output = output + image
    return output


def main():
    camera = cv2.VideoCapture(0)
    impImage = cv2.imread("new.jpg")
    mtx, dist = calibrateCamera()

    while True:
        status, image = camera.read()

        detected_tag = detectTag(mtx,dist, image)

        if len(detected_tag[0]) != 0:
            for frame, tag, rvec, tvec in zip(detected_tag[0], detected_tag[1], detected_tag[2],detected_tag[3]):
                #image = superimposeImage(image, impImage, frame, tag)
                if tag is not None:
                    for i in range(tag.size):
                        upper_left = frame[0][0][0], frame[0][0][1]
                        upper_right = frame[0][1][0], frame[0][1][1]
                        lower_right = frame[0][2][0], frame[0][2][1]
                        lower_left = frame[0][3][0], frame[0][3][1]

                        frame_pos = frame.reshape((4, 2))

                        (up_left, up_right, low_right, low_left) = frame_pos
                        up_left = (int(up_left[0]), int(up_left[1]))

                        #rr, thet = ra.rArea(corners)
                        print(frame)
                        aruco.drawAxis(image, mtx, dist, rvec[0], tvec[0], 0.06)  # np.array([0.0, 0.0, 0.0])
                        cv2.putText(image,
                                    "%.1f cm" % ((tvec[:,2] * 100) - 10),
                                    (up_left[0], up_left[1] - 10), cv2.FONT_HERSHEY_SIMPLEX, 0.5, (244, 244, 244),2)
                        #cv2.circle(image, (100, int(rr / 600)), 6, (200, 40, 230), -1)
                        R, _ = cv2.Rodrigues(rvec[0])
                        cameraPose = -R.T * tvec[0]

        cv2.imshow("Image", image)
        cv2.waitKey(1)


if __name__ == "__main__":
    main()
