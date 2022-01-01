
const connectButton = document.getElementById ('connect-button');
const readButton = document.getElementById ('read-button');
let port;
let decoder;
let reader;
let serialData = new Uint8Array(28); //koliko bajtov bomo prejeli
let serialDataHold = new Uint8Array(28);
let newSerialData = false;

function getSerialData(){
    if(serialData.length == 28){serialDataHold = serialData;}
    return serialDataHold;
}

if ('serial' in navigator) {
    readButton.addEventListener('click', function () {
        console.log(getSerialData());
    });

    connectButton.addEventListener('click', function () {
        if (port) { //zapremo port
            port.close();
            port = undefined;
    
          connectButton.innerText = '🔌 Connect';
        }
        else { //port se ni definiran odpri novega
          getReader();
        }
    });
  
    connectButton.disabled = false;
  }
  else {
      alert("no serial API support in this browser sorry");
  }

async function getReader() {
    try {
        port = await navigator.serial.requestPort({});
        await port.open({ baudRate: 115200 });
        connectButton.innerText = '🔌 Disconnect';
        readButton.disabled = false;

        decoder = new TextDecoder();
        reader = port.readable.getReader();
        while(true){
            const { value, done } = await reader.read();
            //serialData = decoder.decode(value);
            serialData = value; newSerialData = true;
        }
        // Continue connecting to |port|.
    } catch (e) {
        // Permission to access a device was denied implicitly or explicitly by the user.
        if (port) { //zapremo port
            port.close();
            port = undefined;
        }
        port = null;
        connectButton.innerText = '🔌 Connect'
        alert(e);
    }
  }