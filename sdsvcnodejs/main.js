

var http = require('http');
var http_proxy = require('http-proxy');
var url = require('url');
var process = require('child_process');
var g_sdserv_cmd = "c:\\appsrc\\trunk\\debugu\\sdserv.exe";
var g_map = {};
var g_next_port = 9000;

function fetchPort(group, callback)
{
    if (g_map.hasOwnProperty(group))
    {
        var o = g_map[group];
        
        if (o.status == 'r' /* ready */)
        {
            callback(o.port);
        }
         else
        {
            var flagCheck = setInterval(function() {
                if (o.status == 'r') {
                    clearInterval(flagCheck);
                    callback(o.port);
                }
            }, 100); // interval set at 100 milliseconds
        }
    }
     else
    {
        var port = g_next_port;
        g_next_port++;
        
        // --win32evt-ready xxx --win32evt-notready xxx 
        
        var cmdline = g_sdserv_cmd + " -d " + group + " -p " + port + " --idle-quit 3000 ";
        var cmd = "c:\\Program Files\\nodejs\\node.exe";
        var args = [ "C:\\appsrc\\trunk\\strata\\sdsvc\\nodejs\\test.js", port ];

        
        console.log("running " + cmdline);
        
        var ls = process.spawn(cmd, args);
        
        ls.stdout.on('data', function (data) {
            console.log('stdout: ' + data);
        });
        ls.stderr.on('data', function (data) {
            console.log('stderr: ' + data);
        });
        ls.on('close', function (code) {
            console.log('child process exited with code ' + code);
            delete g_map[group];
        });
        
        g_map[group] = { status: 'r', port: port };
        callback(port);
    }

}


var proxy = http_proxy.createProxyServer({});


var http_callback = function (request, response) {
    var url_parsed = url.parse(request.url,true);
    var url_parts = url_parsed.pathname.split('/');

    if (url_parts.length == 0)
    {
        response.writeHead(404, {"Content-Type": "text/plain"});
        response.end('{ "success": false, "msg": "Not found" }');
    }
     else
    {
        var group = url_parts[1];
        
        if (group.substr(0,7).toLowerCase() == 'favicon')
        {
            response.writeHead(404, {"Content-Type": "text/plain"});
            response.end('{ "success": false, "msg": "Not found" }');
            return;
        }
        
        fetchPort(group, function(port) {
            console.log("forwarding to http://localhost:" + port);
            proxy.web(request, response, { target: 'http://localhost:' + port });
        });
    }
};

var server = http.createServer(http_callback);
server.listen(8080);






