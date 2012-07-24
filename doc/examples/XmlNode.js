

//(EXAMPLE)
//Description: Example 1
//Class: XmlNode
//Code:


// a simple parsing example
var test ="
<?xml version='1.0'>
<sample>
   <test1 prop1='value1'>Contents</test1>
   <test2></test2>
   <test3/>
</sample>
";

var node = new XmlNode;
node.parse(test);

alert("Root tag name is: " + node.getNodeName());

var child_nodes = node.getChildNodes();
for (var i in child_nodes)
{
    alert("child " + i +
               " has a tag name of: " +
               child_nodes[i].getNodeName());
               
    if (child_nodes[i].hasAttributes())
    {
        var attrs = child_nodes[i].getAttributes();
        for (var a in attrs)
        {
            alert("child with node name " +
                       child_nodes[i].getNodeName() +
                       " has an attribute " + a +
                       " with a value of " + attrs[a]);
        }
    }              
}


//(/EXAMPLE)

