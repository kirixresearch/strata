
var args = process.argv.slice(2);
var http = require('http');
var g_port = args[0];

var http_callback = function (request, response) {
    response.writeHead(200, {"Content-Type": "text/plain"});
    response.end("Url " + request.url + " Port " + g_port);
};

var server = http.createServer(http_callback);
server.listen(g_port);

