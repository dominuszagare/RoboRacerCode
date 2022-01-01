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
    q0: 1,
    q1: 0,
    q2: 0,
    q3: 0,
    pitch: 0,
    yaw: 0,
    roll: 0,
}

var JSdata
        
var intervalID = setInterval(update_values,100);
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
            }
    );
    var dir = new THREE.Quaternion(res.q2[0],res.q3[0],res.q1[0],res.q0[0]); //zamenjal sem y in z da model narisem pokoncno
    robot0.setRotationFromQuaternion(dir);
    arrowHelperW.setRotationFromQuaternion(dir);
    debugText.innerHTML = "pitch " + String(res.pitch) + "<br> roll " + String(res.roll) + "<br> yaw " + String(res.yaw);
}

var update = function(){
    //izracunja orentacijo
};

var RenderLoop = function(){
    requestAnimationFrame(RenderLoop);
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
