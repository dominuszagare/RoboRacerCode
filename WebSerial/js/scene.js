import * as THREE from "./libs/threejs/three.module.js";
import { OrbitControls } from "./libs/threejs/controls/OrbitControls.js";
import { GLTFLoader } from "./libs/threejs/loaders/GLTFLoader.js";

var scene = new THREE.Scene();
var camera = new THREE.PerspectiveCamera(75,window.innerWidth / window.innerHeight,0.1,1000);
var renderer = new THREE.WebGLRenderer();
renderer.setSize(window.innerWidth,window.innerHeight);
document.body.appendChild(renderer.domElement);
var debugText = document.getElementById("info");

var controls = new OrbitControls(camera,renderer.domElement);
const loader = new GLTFLoader();


const Ambientlight = new THREE.AmbientLight( 0x202020 ); // soft white light
scene.add( Ambientlight );
const light = new THREE.PointLight( 0xffffff, 1, 90 );
light.position.set( 2, 10, -2 );
scene.add( light );

camera.position.z = 2;
camera.position.y = 2;
camera.position.x = 2;
camera.lookAt(0,0,0);

const arrowHelperX = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xff0000 );
const arrowHelperY = new THREE.ArrowHelper( new THREE.Vector3( 0, 1, 0) , new THREE.Vector3( 0, 0, 0), 1, 0x00ff00 );
const arrowHelperZ = new THREE.ArrowHelper( new THREE.Vector3( 0, 0, 1) , new THREE.Vector3( 0, 0, 0), 1, 0x0000ff );
const arrowHelperW = new THREE.ArrowHelper( new THREE.Vector3( 1, 0, 0) , new THREE.Vector3( 0, 0, 0), 1, 0xffff00 );
scene.add( arrowHelperX );
scene.add( arrowHelperY );
scene.add( arrowHelperZ );
scene.add( arrowHelperW );

const geometry = new THREE.BoxGeometry(0.3,0.05,0.5);
const robot0 = new THREE.Mesh( geometry, new THREE.MeshStandardMaterial( { color: 0x00ff0f } ));

scene.add( robot0 );


let clock = new THREE.Clock();
let delta = 0;

var IMUdata = {
    AccX: 0.0,
    AccY: 0.0,
    AccZ: 0.0,
    GyroX: 0.0,
    GyroY: 0.0,
    GyroZ: 0.0,
    MagX: 0.0,
    MagY: 0.0,
    MagZ: 0.0,
};

var res ={
    pitch: 0,
    yaw: 0,
    roll: 0,
}

var update = function(){
    //izracunja orentacijo
    delta += clock.getDelta();
    if(delta > 0.04){ //25fps
        var data = new Float32Array(getSerialData().buffer);
        //var dataint16 = new Int16Array(getSerialData().buffer);
        /*
        var head = 0
        var num = 0
        IMUdata.AccX = dataint16[2]//*0.000122; //1g pospeska
        IMUdata.AccY = dataint16[3]
        IMUdata.AccZ = dataint16[4]
        IMUdata.MagX = dataint16[8]// *0.0015; //1gauss
        IMUdata.MagY = dataint16[9]
        IMUdata.MagZ = dataint16[10]
        IMUdata.GyroX = dataint16[5];
        IMUdata.GyroY = dataint16[6];
        IMUdata.GyroZ = dataint16[7];
        

        var accV = new THREE.Vector3( IMUdata.AccX, -IMUdata.AccZ, IMUdata.AccY); //vektor gravitacije
        var magV = new THREE.Vector3( IMUdata.MagX, IMUdata.MagZ, IMUdata.MagY).normalize();
        
        var dolV = accV.clone(); dolV.normalize(); //vektor gravitacije
        var desnoV = magV.clone(); desnoV.cross(dolV); desnoV.normalize();
        //cros produkt med pospeskom in magnetnim pojem nam da vzhod
        var naprejV = desnoV.clone(); naprejV.cross(dolV); naprejV.normalize();
        //krizni produkt med vzhodom in dol nam da sever

        arrowHelperW.setDirection(new THREE.Vector3(magV.x,magV.y,magV.z));

        robot0.lookAt(naprejV);

        arrowHelperX.setDirection(new THREE.Vector3(naprejV.x,naprejV.y,naprejV.z));
        arrowHelperZ.setDirection(new THREE.Vector3(dolV.x,dolV.y,dolV.z));
        arrowHelperY.setDirection(new THREE.Vector3(desnoV.x,desnoV.y,desnoV.z));
        //console.log(IMUdata.GyroX,IMUdata.GyroY,IMUdata.GyroZ,IMUdata.MagX,IMUdata.MagY,IMUdata.MagZ);
        */
       var dir = new THREE.Quaternion(data[4],data[6],data[5],data[3]); //zamenjal sem y in z da model narisem pokoncno
       robot0.setRotationFromQuaternion(dir);
       arrowHelperW.setRotationFromQuaternion(dir);
       debugText.innerHTML = "tilt " + String(data[0]) + "<br> roll " + String(data[1]) + "<br> yaw " + String(data[2]);
        

        delta = delta%0.04;
    }
};

var RenderLoop = function(){
    requestAnimationFrame(RenderLoop);
    update();
    renderer.render(scene,camera);
};

/*
loader.load( '../models/robot.glb', function ( gltf ) {
	scene.add( gltf.scene );
}, undefined, function ( error ) {
	console.error( error );
} );
*/
RenderLoop();
