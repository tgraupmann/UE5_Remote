const WebSocket = require('ws');

const wss = new WebSocket.WebSocketServer({ port: 8080 });

function tryJsonParse(str) {
  try {
    return JSON.parse(str);
  } catch (e) {
    return undefined;
  }
}

function ab2str(buf) {
  return String.fromCharCode.apply(null, new Uint16Array(buf));
}


wss.on('connection', function connection(ws, req) {
  console.log(new Date(), 'Connection opened', req.url);
  ws._socket.url = req.url; //save url on the socket
  ws.on('message', async function message(data) {
    let json = tryJsonParse(data);
    let input = false;
    if (json != undefined) {
      input = true;
      //console.log('onMessage:', ab2str(data));
    }
    //let count = 0;
    wss.clients.forEach(function each(client) {
      //console.log(new Date(), 'client', client._socket.url);
      if (input) {
        if (client._socket.url == '/?type=input') {
          if (client !== ws && client.readyState === WebSocket.OPEN) {
            //console.log(new Date(), 'Send input to client', client._socket.url, data.length, 'bytes');
            client.send(data);
          }
        }
      } else {
        if (client._socket.url.startsWith('/?i=')) {
          if (client !== ws && client.readyState === WebSocket.OPEN) {
            //console.log(new Date(), 'Send image to client', client._socket.url, data.length, 'bytes');
            client.send(data);
            //++count;
          }
        }
      }
    });
    //console.log(new Date(), 'Send data to', count, 'clients.');
  });
  ws.on('error', function (err) {
    console.error(new Date(), 'WSS Error', err);
  });
});

wss.on('error', function (err) {
  console.error(new Date(), 'WSS Error', err);
});
