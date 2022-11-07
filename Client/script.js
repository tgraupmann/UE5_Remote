console.log('Script version 1.0');

var countFPS = 0;
var dataPerSecond = 0;

// ref: https://stackoverflow.com/questions/15900485/correct-way-to-convert-size-in-bytes-to-kb-mb-gb-in-javascript
function formatBytes(a, b = 2) { if (0 === a) return "0 Bytes"; const c = 0 > b ? 0 : b, d = Math.floor(Math.log(a) / Math.log(1024)); return parseFloat((a / Math.pow(1024, d)).toFixed(c)) + " " + ["Bytes", "KB", "MB", "GB", "TB", "PB", "EB", "ZB", "YB"][d] }

setInterval(function () {
  lblFPS.innerText = countFPS;
  countFPS = 0;
  lblBytes.innerText = formatBytes(dataPerSecond);
  dataPerSecond = 0;
}, 1000);

function openFullscreen() {
  if (divFullScreen.requestFullscreen) {
    divFullScreen.requestFullscreen();
  } else if (divFullScreen.webkitRequestFullscreen) { /* Safari */
    divFullScreen.webkitRequestFullscreen();
  } else if (divFullScreen.msRequestFullscreen) { /* IE11 */
    divFullScreen.msRequestFullscreen();
  }
  divFullScreen.style.cursor = 'none';
  divFullScreen.requestPointerLock();
}

async function handleOnMessage(event) {
  let data = event.data;

  let buffer = await data.arrayBuffer();
  dataPerSecond += Number(buffer.byteLength);

  let reader = new FileReader();
  reader.onload = function (e2) {
    let url = e2.target.result;
    imgVideo.src = url;
    ++countFPS;
  }
  reader.readAsDataURL(data); // read the blob into a url
}

var streamSockets = {};

function connectStreamSocket(name, index) {
  if (streamSockets[name] == undefined) {
    var url = "ws://localhost:8080?i=" + index;
    streamSockets[name] = new WebSocket(url);
    streamSockets[name].onopen = function (event) {
      console.log(new Date(), 'WebSocket connected!', url);
      streamSockets[name].onmessage = function (event) {
        handleOnMessage(event);
      }
    };
    streamSockets[name].onclose = function (event) {
      streamSockets[name] = undefined;
      setTimeout(function () {
        // reconnect after delay
        connectStreamSocket(name, index);
      }, 5000);
    };
    streamSockets[name].onerror = function (error) {
      console.error('streamSocket error! ', error);
      setTimeout(function () {
        // reconnect after delay
        connectStreamSocket(name, index);
      }, 5000);
    };
  }
}

document.body.addEventListener('keydown', function (evt) {
  //console.log(new Date(), 'keydown', evt.key);
  if (evt.key == 'Escape') {
    divFullScreen.style.cursor = '';
    divFullScreen.exitPointerLock();
    return;
  }
  let streamSocket = streamSockets['main'];
  if (!streamSocket || streamSocket.readyState != WebSocket.OPEN) {
    return; // connection closed
  }
  let sendJson;
  switch (evt.key.toLowerCase()) {
    case 'w':
    case 'a':
    case 's':
    case 'd':
      //console.log(new Date(), 'keydown', evt.key);
      sendJson = JSON.stringify({
        input: "keydown",
        key: evt.key
      });
      streamSocket.send(sendJson);
      break;
    case ' ':
      //console.log(new Date(), 'keydown', 'space');
      sendJson = JSON.stringify({
        input: "keydown",
        key: "space"
      });
      streamSocket.send(sendJson);
      break;
  }
});

document.body.addEventListener('keyup', function (evt) {
  //console.log(new Date(), 'keyup', evt.key);
  if (evt.key == 'Escape') {
    divFullScreen.style.cursor = '';
    divFullScreen.exitPointerLock();
    return;
  }
  let streamSocket = streamSockets['main'];
  if (!streamSocket || streamSocket.readyState != WebSocket.OPEN) {
    return; // connection closed
  }
  let sendJson;
  switch (evt.key.toLowerCase()) {
    case 'w':
    case 'a':
    case 's':
    case 'd':
      //console.log(new Date(), 'keydown', evt.key);
      sendJson = JSON.stringify({
        input: "keyup",
        key: evt.key
      });
      streamSocket.send(sendJson);
      break;
    case ' ':
      //console.log(new Date(), 'keydown', 'space');
      sendJson = JSON.stringify({
        input: "keyup",
        key: "space"
      });
      streamSocket.send(sendJson);
      break;
  }
});

var lastX = undefined;
var lastY = undefined;
var mouseX = undefined;
var mouseY = undefined;

// limit rate input events
setInterval(function () {
  let streamSocket = streamSockets['main'];
  if (!streamSocket || streamSocket.readyState != WebSocket.OPEN) {
    return; // connection closed
  }
  if (mouseX != 0 || mouseY != 0) {
    if (mouseX != lastX || mouseY != lastY) {
      lastX = mouseX;
      lastY = mouseY;
      let sendJson = JSON.stringify({
        input: "mouse",
        x: mouseX,
        y: mouseY,
      });
      //console.log('send', sendJson);
      streamSocket.send(sendJson);
      mouseX = 0;
      mouseY = 0;
    }
  }
}, 1000 / 60);

divFullScreen.addEventListener('mousemove', function (evt) {
  //console.log('mouseover', evt.movementX, evt.movementY, evt);
  mouseX += evt.movementX / 10;
  if (Number.isNaN(mouseX)) {
    mouseX = 0;
  }
  mouseY += evt.movementY / 10;
  if (Number.isNaN(mouseY)) {
    mouseY = 0;
  }
});

connectStreamSocket('main', 0);
//connectStreamSocket('connection1', 1);
//connectStreamSocket('connection2', 2);
