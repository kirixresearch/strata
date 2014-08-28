

var http = require('http');
var http_proxy = require('http-proxy');
var url = require('url');
var process = require('child_process');
var g_sdserv_cmd = "D:\\build32\\src\\trunk\\releaseu\\sdserv.exe";
var g_map = {};
var g_next_port = 9000;

function fetchPort(group, callback)
{
    var info;
    
    if (g_map.hasOwnProperty(group))
    {
        info = g_map[group];
    }
     else
    {
        info = { "status": "x", "port": g_next_port };
        ++g_next_port;
        g_map[group] = info;
    }
        
    if (info.status == 'r' /* ready */)
    {
        callback(info.port);
        return;
    }
     else if (info.status == 'l')
    {
        var flagCheck = setInterval(function() {
            if (info.status == 'r') {
                clearInterval(flagCheck);
                callback(info.port);
                return;
            }
        }, 100); // interval set at 100 milliseconds
        return;
    }
     else
    {
        // --win32evt-ready xxx --win32evt-notready xxx 
        
        var cmdline = g_sdserv_cmd + " -d " + group + " -p " + info.port + " --idle-quit 3000 ";
        
        var cmd = "D:\\build32\\src\\trunk\\releaseu\\sdserv.exe";
        var args = [ "-d", group, "-p", info.port, "--idle-quit", "3000" ];

        
        //var cmd = "c:\\Program Files\\nodejs\\node.exe";
        //var args = [ "D:\\build32\\src\\trunk\\sdsvcnodejs\\test.js", info.port ];

        
        console.log("running " + cmd + args.join(' '));
        
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
        
        info.status = 'r';
        callback(info.port);
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
        
        console.log(request.url);
        
        var question = request.url.indexOf('?');
        
        var new_url;
        if (question == -1)
            new_url = request.url;
             else
            new_url = request.url.substr(0, question);
        
        var slash = new_url.indexOf('/',1);
        if (slash == -1)
            new_url = '/';
             else
            new_url = new_url.substr(slash);
        if (question != -1)
            new_url += request.url.substr(question);
        request.url = new_url;
        console.log(request.url);
        
        fetchPort(group, function(port) {
            console.log("forwarding to http://localhost:" + port);
            proxy.web(request, response, { target: 'http://localhost:' + port });
        });
    }
};

var server = http.createServer(http_callback);
server.listen(8080);






