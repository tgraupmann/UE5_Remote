const fs = require('fs'); //debugging
const WebSocket = require('ws');

const wss = new WebSocket.WebSocketServer({ port: 8080 });

wss.on('connection', function connection(ws) {
  ws.on('message', function message(data) {
    console.log(new Date(), 'onMessage', data)
    wss.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        console.log(new Date(), 'Send data to clients', data.length, 'bytes');
        client.send(data);

        /*
        fs.writeFile('test.png', data, null, function (err) {
          if (err) {
            return console.log(new Date(), 'Failed to write!', err);
          }
        });
        */
      }
    });
  });
  ws.on('error', function (err) {
    console.error(new Date(), 'WSS Error', err);
  });
});

wss.on('error', function (err) {
  console.error(new Date(), 'WSS Error', err);
});
