console.log('Script version 1.0');

var countFPS = 0;

setInterval(function () {
  lblFPS.innerText = countFPS;
  countFPS = 0;
}, 1000);

function handleOnMessage(event) {
  let data = event.data;

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
