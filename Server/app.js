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

wss.on('connection', function connection(ws) {
  ws.on('message', function message(data) {
    let json = tryJsonParse(data);
    if (json != undefined) {
      console.log('onMessage:', ab2str(data));
    }
    wss.clients.forEach(function each(client) {
      if (client !== ws && client.readyState === WebSocket.OPEN) {
        //console.log(new Date(), 'Send data to clients', data.length, 'bytes');
        client.send(data);
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
