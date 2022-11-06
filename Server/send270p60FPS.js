const fs = require('fs');
const WebSocket = require('ws');

const totalConnections = 3;
var fps = 0;

// ref: https://stackoverflow.com/questions/15900485/correct-way-to-convert-size-in-bytes-to-kb-mb-gb-in-javascript
function formatBytes(a, b = 2) { if (0 === a) return "0 Bytes"; const c = 0 > b ? 0 : b, d = Math.floor(Math.log(a) / Math.log(1024)); return parseFloat((a / Math.pow(1024, d)).toFixed(c)) + " " + ["Bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"][d] }

const data = fs.readFileSync('images/image_1.png');
console.log('Image length', data.length);

setInterval(function () {
  fps = 0;
}, 1000);

for (var i = 0; i < totalConnections; ++i) {

  const ws = new WebSocket('ws://localhost:8080?type=host');

  const refI = i;
  ws.on('open', function open() {

    console.log(new Date(), 'Connection opened', refI);

    this.dataPerSecond = 0;

    var refThis = this;

    try {

      setInterval(function () {

        if (fps < 60) {
          ws.send(data);
          refThis.dataPerSecond += data.length;
          //console.log(new Date(), 'Connection', refI, 'Data sent!', data.length, 'bytes');
        }

        ++fps;

      }, 1000 / 60 * totalConnections * 0.8); // 60 FPS

      setInterval(function () {
        console.log(new Date(), 'Connection', refI, 'Sent', formatBytes(refThis.dataPerSecond));
        refThis.dataPerSecond = 0;
      }, 1000);

    } catch (err) {
      console.error(err);
    }
  });

}
