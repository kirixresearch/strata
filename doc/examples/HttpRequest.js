

//(EXAMPLE)
//Description: Example 1
//Class: HttpRequest
//Code:


var http;

// getting a web page
http = new HttpRequest;
http.setMethod(HttpRequest.methodGet);
http.setUrl("http://my.sample.domain.com");
http.send();
alert(http.getResponseText());

// an example demonstrating basic authentication
http = new HttpRequest;
http.setMethod(HttpRequest.methodGet);
http.setUrl("http://my.sample.domain.com");
http.setBasicAuth("test", "this");
http.send();
alert(http.getResponseText());

// an example post method call
http = new HttpRequest;
http.setMethod(HttpRequest.methodPost);
http.setUrl("http://my.sample.domain.com");
http.setPostValue("form_element1", "123");
http.setPostValue("form_element2", "456");
http.send();
alert(http.getResponseText());


//(/EXAMPLE)

