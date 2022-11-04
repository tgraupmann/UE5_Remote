const fs = require('fs');
const WebSocket = require('ws');

const ws = new WebSocket('ws://localhost:8080');

ws.on('open', function open() {

  try {
    const data = fs.readFileSync('images/image_1.png');
    console.log('Image length', data.length);
    ws.send(data);
    console.log(new Date(), 'Data sent!', data.length, 'bytes');
  } catch (err) {
    console.error(err);
  }

  setTimeout(function () {
    console.log(new Date(), 'Disconnect!');
    ws.close();
  }, 1000);
});
