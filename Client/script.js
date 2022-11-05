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

var streamSocket = null;

function connectStreamSocket() {
  if (streamSocket == undefined) {
    var url = "ws://localhost:8080";
    streamSocket = new WebSocket(url);
    streamSocket.onopen = function (event) {
      console.log(new Date(), 'WebSocket connected!', url);
      streamSocket.onmessage = function (event) {
        handleOnMessage(event);
      }
    };
    streamSocket.onclose = function (event) {
      streamSocket = undefined;
      setTimeout(function () {
        // reconnect after delay
        connectStreamSocket();
      }, 5000);
    };
    streamSocket.onerror = function (error) {
      console.error('streamSocket error! ', error);
      setTimeout(function () {
        // reconnect after delay
        connectStreamSocket();
      }, 5000);
    };
  }
}

document.body.addEventListener('keydown', function (evt) {
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

divFullScreen.addEventListener('mousemove', function (evt) {
  if (!streamSocket || streamSocket.readyState != WebSocket.OPEN) {
    return; // connection closed
  }
  console.log('mouseover', evt.offsetX, evt.offsetY, evt);
  if (evt.offsetX != 0 && evt.offsetY != 0) {
    let sendJson = JSON.stringify({
      input: "mouse",
      x: evt.offsetX,
      y: evt.offsetY,
    });
    streamSocket.send(sendJson);
  }
});

connectStreamSocket();
