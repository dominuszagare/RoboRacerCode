import * as THREE from "./libs/threejs/three.module.js";
import { OrbitControls } from "./libs/threejs/controls/OrbitControls.js";
import { GLTFLoader } from "./libs/threejs/loaders/GLTFLoader.js";


var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75,window.innerWidth / window.innerHeight,0.1,1000);
var renderer = new THREE.WebGLRenderer();
renderer.shadowMap.enabled = true; //omogocimo sence

renderer.setSize(window.innerWidth,window.innerHeight);
document.body.appendChild(renderer.domElement);
var debugText = document.getElementById("info");

var controls = new OrbitControls(camera,renderer.domElement);
const loader = new GLTFLoader();
var robo = new THREE.Object3D();
var ovira = new THREE.Object3D();

loader.load( '../models/robot.glb', function ( gltf ) {
    robo = gltf.scene.children[0];
	scene.add( robo );
    robo.matrixAutoUpdate = false;

}, undefined, function ( error ) {
	console.error( error );
} );

loader.load( '../models/ovira.glb', function ( gltf ) {
    ovira = gltf.scene.children[0];
	scene.add( ovira );
    ovira.visible = false;
}, undefined, function ( error ) {
	console.error( error );
} );


//poscaj sled za vozilom
const trailMaterial1 = new THREE.LineBasicMaterial( {
    color: 0x0f00f,
} );
var trailPath = [];
trailPath.push( 
  new THREE.Vector3( 0.0, 0.0, 0.0 ), 
);
const TrailLine = new THREE.Line( new THREE.BufferGeometry().setFromPoints( trailPath ), trailMaterial1 );
scene.add( TrailLine );



var pi = 3.14159265358979323846

//osvetlitev scene
scene.background = new THREE.Color(0xa0a0a0);
var light = new THREE.HemisphereLight(0xffeeb1,0x080802,4);
scene.add( light );
var spotlight = new THREE.SpotLight(0x303030,4);
spotlight.position.set(-50,50,50);
spotlight.castShadow = true;
scene.add( spotlight );


camera.position.z = 8;
camera.position.y = 8;
camera.position.x = 8;
camera.lookAt(0,0,0);

const arrowHelperX = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xff0000 );
const arrowHelperY = new THREE.ArrowHelper( new THREE.Vector3( 0, 1, 0) , new THREE.Vector3( 0, 0, 0), 1, 0x00ff00 );
const arrowHelperZ = new THREE.ArrowHelper( new THREE.Vector3( 0, 0, 1) , new THREE.Vector3( 0, 0, 0), 1, 0x0000ff );
const arrowHelperW = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xffff00 );
const arrowHelperGx = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xff00f0 );
const arrowHelperGy = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xff0f00 );
const arrowHelperR1 = new THREE.ArrowHelper( new THREE.Vector3( -1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0x0f00f0 );

scene.add( arrowHelperX );
scene.add( arrowHelperY );
scene.add( arrowHelperZ );
scene.add( arrowHelperW );
scene.add( arrowHelperGx );
scene.add( arrowHelperGy );
scene.add( arrowHelperR1 );

//mreza za tla
const gridHelper = new THREE.GridHelper( 100, 100 );
gridHelper.position.copy(new THREE.Vector3(0,-0.6,0));
scene.add( gridHelper );


//zelena skatlica ploscica STM32
const robot0 = new THREE.Mesh( new THREE.BoxGeometry(0.6,0.05,0.9), new THREE.MeshStandardMaterial( { color: 0x00ff0f } ));
scene.add( robot0 );



var targetObjecs = [];
var targetPozX = [];
var targetPozY = [];
var furstrumArea = [];

var CAMERA_HALF_RESX = 300; //600x300 resolucija
var CAMERA_HALF_RESY = 150;
var CAMERA_PROJECTION_PLANE = 800;

//za prikaz gledisca kamere
furstrumArea.push(new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 12, 0x00f0f0 ));
furstrumArea.push(new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 12, 0x00f0f0 ));
furstrumArea.push(new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 12, 0x00f0f0 ));
furstrumArea.push(new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 12, 0x00f0f0 ));

targetObjecs.push(new THREE.Mesh( new THREE.BoxGeometry(0.01,0.01,0.01), new THREE.MeshStandardMaterial( { color: 0x0f0ff0 } ))); 
targetPozX.push(CAMERA_HALF_RESX); targetPozY.push(CAMERA_HALF_RESY); //od x in y piksla kamere odstejemo polovico resolucije slike
targetObjecs.push(new THREE.Mesh( new THREE.BoxGeometry(0.01,0.01,0.01), new THREE.MeshStandardMaterial( { color: 0x0f0ff0 } ))); 
targetPozX.push(-CAMERA_HALF_RESX); targetPozY.push(CAMERA_HALF_RESY);
targetObjecs.push(new THREE.Mesh( new THREE.BoxGeometry(0.01,0.01,0.01), new THREE.MeshStandardMaterial( { color: 0x0f0ff0 } ))); 
targetPozX.push(-CAMERA_HALF_RESX); targetPozY.push(-CAMERA_HALF_RESY);
targetObjecs.push(new THREE.Mesh( new THREE.BoxGeometry(0.01,0.01,0.01), new THREE.MeshStandardMaterial( { color: 0x0f0ff0 } ))); 
targetPozX.push(CAMERA_HALF_RESX); targetPozY.push(-CAMERA_HALF_RESY);
targetObjecs.forEach(obj => {
    scene.add(obj);
});
furstrumArea.forEach(obj => {
    scene.add(obj);
});

let clock = new THREE.Clock();

var razdaljaOdovire = 2.4;
var res ={
    q0: 1,
    q1: 0,
    q2: 0,
    q3: 0,
    pitch: 0,
    yaw: 0,
    roll: 0,
    pozX: 0,
    pozY: 0,
    magx: 0,
    magy: 0,
    magz: 0,
    podatki_senzorja: [],
    razsirjeniPodatki: []
}

var JSdata
        
var intervalID = setInterval(update_values,500);
function update_values() {
    $.getJSON("_getData",
            function (data) {
                //console.log(data)
                JSdata = data;
                res.roll = data.roll;
                res.yaw = data.yaw;
                res.pitch = data.pitch;
                res.q0 = data.q0;
                res.q1 = data.q1;
                res.q2 = data.q2;
                res.q3 = data.q3;
                res.pozX = data.pozX;
                res.pozY = data.pozY;
                res.gx = data.gX;
                res.gy = data.gY;
                res.gz = data.gZ;
                res.podatki_senzorja = data.podatki_senzorja;
            }
    );
    //var dir = new THREE.Quaternion(res.q2[0],res.q3[0],res.q1[0],res.q0[0]); //zamenjal sem y in z da model narisem pokoncno
    
    //odkomentriaj za debugiranje
    
    var dir = new THREE.Quaternion(0,0.1,0,1);
    res.pozX = 1.2; res.pozY = 2; res.gx=0.1; res.gy = 0.2; res.gz = 1

    var ROBOTPOZ = new THREE.Vector3( res.pozX[0], 0.2, res.pozY[0]);

    arrowHelperW.setRotationFromQuaternion(dir);
    arrowHelperW.position.copy(ROBOTPOZ);

    

    
    var localOffset = new THREE.Vector3(-0.8,-0.4,0);
    var eulerAngles = new THREE.Euler(0,1,0);
    eulerAngles.setFromQuaternion(dir);
    //console.log(eulerAngles);
    localOffset.applyQuaternion(dir)
  
    var trailPoint = new THREE.Vector3();
    trailPoint.copy(ROBOTPOZ);
    trailPoint.addVectors(trailPoint,localOffset);

    trailPath.push( 
        trailPoint 
    );
    if(trailPath.length > 200){
        trailPath.shift();
    }
    TrailLine.geometry = new THREE.BufferGeometry().setFromPoints( trailPath );


    robot0.setRotationFromQuaternion(dir);
    robot0.position.copy( ROBOTPOZ);


    var rotationMatrix = new THREE.Matrix4();

    robo.matrix.makeRotationY((-90*pi)/180);
    rotationMatrix.makeRotationFromQuaternion(dir);
    robo.applyMatrix4(rotationMatrix);
    robo.matrix.scale(new THREE.Vector3( 0.01, 0.01, 0.01))
    robo.matrix.setPosition(ROBOTPOZ);

    //rotationMatrix.makeRotationFromQuaternion(dir);
    //robo.matrix = robo.matrix * rotationMatrix;
    //robo.matrix = robo.matrix * THREE.Matrix4.makeRotationY((90*pi)/180);
    //roboMatrix.copy(rotationMatrix);
    //roboMatrix.setPosition(new THREE.Vector3( res.pozX, 0, res.pozY));
    //robo.applyMatrix4(roboMatrix);


    //arrowHelperG.setDirection(new THREE.Vector3( res.magx, res.magz, res.magy))
    
    arrowHelperGx.setRotationFromQuaternion(dir);
    arrowHelperGx.rotateX((90*pi)/180);
    arrowHelperGx.setLength(res.gx*4,res.gx,0.1)
    arrowHelperGx.position.copy(ROBOTPOZ);
    arrowHelperGx.translateZ(-0.1);

    arrowHelperGy.setRotationFromQuaternion(dir);
    arrowHelperGy.rotateZ((90*pi)/180);
    arrowHelperGy.setLength(res.gy*4,res.gy,0.1)
    arrowHelperGy.position.copy(ROBOTPOZ);
    arrowHelperGy.translateX(0.1);
    
    //arrowHelperG.matrix.makeRotationFromQuaternion(dir);
    //arrowHelperG.applyMatrix4(rotationMatrix);
    
    //arrowHelperMag.position.copy(robot0.position)
    if(res.podatki_senzorja.length > 6){
        res.razsirjeniPodatki = RazsiriPodatke(res.podatki_senzorja);
    }

    debugText.innerHTML = "pitch " + String(res.pitch*(180/pi)) + "<br> roll " + String(res.roll*(180/pi)) + "<br> yaw " + String(res.yaw*(180/pi))
    + "<br> pozX" + String(res.pozX) +"<br> pozY"+ String(res.pozY)
    + "<br> X " + String(res.gx) + "<br>Y "  + String(res.gy) + "<br>Z "  + String(res.gz)
    + "<br> Razdalja1 " + String(res.razsirjeniPodatki[0]); //+ "<br>Razdalja2 "  + String(res.razsirjeniPodatki[1]);


    
    ovira.position.copy(ROBOTPOZ)
    ovira.setRotationFromQuaternion(dir);
    rotationMatrix.makeRotationY((-90*pi)/180);
    var translationMatrix = new THREE.Matrix4();
    translationMatrix.makeTranslation(razdaljaOdovire+1.2,0,0);
    ovira.applyMatrix4(translationMatrix)
    ovira.applyMatrix4(rotationMatrix);
    ovira.matrix.scale(new THREE.Vector3( 0.01, 0.01, 0.01));
   
    //izrisi gledisce kamere in zaznane tarce prvi 4 objekti so za dolocanje gledisca
    for (let i = 0; i < targetObjecs.length; i++) {
        targetObjecs[i].setRotationFromQuaternion(dir);
        targetObjecs[i].position.copy(ROBOTPOZ);
        targetObjecs[i].translateX(1.2)
        targetObjecs[i].translateY(0.2)
        var pozition = new THREE.Vector3();
        pozition.copy(targetObjecs[i].position);

        if(i<4){furstrumArea[i].position.copy(targetObjecs[i].position);}
        

        var offsetCamera = new THREE.Vector3(CAMERA_PROJECTION_PLANE,targetPozY[i],targetPozX[i]);
        //var offsetCamera = new THREE.Vector3(1000,200,300);
        offsetCamera.normalize();
        offsetCamera.applyQuaternion(dir);

        if(i<4){furstrumArea[i].setDirection(offsetCamera);}

        offsetCamera.setLength(5);
        pozition.addVectors(pozition,offsetCamera);
        targetObjecs[i].position.copy(pozition);
        
    }

    if(res.razsirjeniPodatki[0] > 0){
        arrowHelperR1.setRotationFromQuaternion(dir);
        arrowHelperR1.rotateZ((-90*pi)/180);
        arrowHelperR1.setLength(res.razsirjeniPodatki[0]*0.01)
        arrowHelperR1.position.copy(ROBOTPOZ);
        arrowHelperR1.translateX(0.3);
        arrowHelperR1.translateY(1.8);
        arrowHelperR1.translateZ(-0.35);
    } 
    
    

} 

var update = function(){
    //izracunja orentacijo
};

var RenderLoop = function(){
    requestAnimationFrame(RenderLoop);
    renderer.render(scene,camera);
};

RenderLoop();
