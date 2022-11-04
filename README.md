# UE5_Remote

POC - Video Streaming with UE5

## Components

### WebSocket Server

The `WebSocket Server` relays streaming binary images to clients.

* Dependency: [websockets/ws](https://github.com/websockets/ws)

* `WebSocket Server` [Server/app.js](Server/app.js)

```
cd Server
npm install
node app.js
```

### HTML5 Client

The HTML5 client uses a WebSocket client to receive binary images and display in an img element. The HTML5 client can also send input to the host by using WebSocket messages.

![image_1](images/image_1.png)

### UE5 Remote Host

The [Unreal Remote Host](UE5_Remote/UE5_Remote.uproject) is an UE5 project that uses a render texture and a WebSocket client to send binary image data to clients. Clients display the binary images and can send inputs back to the host to control the character. The UE5 project extends the third person perspective game template.

## Unit Tests

* [Server/sendImage.js](Server/sendImage.js) sends an image to the `WebSocket Server` that the client displays.

![image_2](images/image_2.png)
