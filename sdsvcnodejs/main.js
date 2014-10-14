

var http = require('http');
var http_proxy = require('http-proxy');
var bouncy = require('bouncy');
var url = require('url');
var process = require('child_process');
var g_sdserv_cmd = "D:\\build32\\src\\trunk\\releaseu\\sdserv.exe";
var g_map = {};
var g_next_port = 9000;

function fetchPort(group, callback, error)
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
			console.log("waiting..."+Date());
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
		// set status to loading
		info.status = 'l';
		
        // --win32evt-ready xxx --win32evt-notready xxx 
        
        var cmdline = g_sdserv_cmd + " -d " + group + " -p " + info.port + " --idle-quit 3000 ";
        
        var cmd = "D:\\build32\\src\\trunk\\releaseu\\sdserv.exe";
        var args = [ "-d", group, "-p", info.port, "-s", 1 /* strip off first part of url */, "--idle-quit", "3000" ];

        
        //var cmd = "c:\\Program Files\\nodejs\\node.exe";
        //var args = [ "D:\\build32\\src\\trunk\\sdsvcnodejs\\test.js", info.port ];

        
        console.log("running " + cmd + " " + args.join(' '));
        
        var ls = process.spawn(cmd, args);
        
        ls.stdout.on('data', function (data) {
            console.log(data);
			if ((''+data).indexOf("*** sdserv ready") != -1)
			{
				// set status to running
			    info.status = 'r';
				callback(info.port);
			}
        });
        ls.stderr.on('data', function (data) {
            console.log('stderr: ' + data);
        });
        ls.on('close', function (code) {
            console.log('child process exited with code ' + code);
			console.log('child process was serving on port ' + info.port);
            info.status = 'x';
        });
        

    }

}




var bouncy = require('bouncy');

var server = bouncy(function (request, response, bounce)
{
    var url_parsed = url.parse(request.url,true);
    var url_parts = url_parsed.pathname.split('/');
	
	var group = '';
    if (url_parts.length > 0)
		group = url_parts[1];
	console.log(JSON.stringify(url_parts));
	if (group.length == 0 || group.substr(0,7).toLowerCase() == 'favicon')
	{
		response.statusCode = 404;
		response.writeHead(404, {"Content-Type": "text/plain"});
		response.end('{ "success": false, "msg": "Not found" }');
		return;
	}
	
		
	console.log(new Date().toISOString() + " - " + request.url + " HTTP " + request.httpVersion);
	console.log(JSON.stringify(request.headers,null,4));
	
/*
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
*/

	fetchPort(group, function(port) {
		console.log("forwarding to http://localhost:" + port);
		bounce(port);
	});

	/*
    else {
		// error
        res.statusCode = 404;
        res.end('no such host');
    }
	*/
});
server.listen(80);


/*
var proxy = http_proxy.createProxyServer({agent:false});


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
        
        console.log(Date() + " - " + request.url +  " " + request.httpVersion + " " + JSON.stringify(request.headers));
        
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
        
        fetchPort(group, function(port) {
            console.log("forwarding to http://localhost:" + port);
            proxy.web(request, response, { target: 'http://localhost:' + port });
        });
    }
};

var server = http.createServer(http_callback);
server.listen(80);

*/




