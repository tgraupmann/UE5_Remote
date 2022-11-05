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

connectStreamSocket();
