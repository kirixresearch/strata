

//(EXAMPLE)
//Description: Example 1
//Class: Directory
//Code:


// test if a directory exists, if it does, list
// the subdirectories in the directory

var dir = "c:\\";
if (!Directory.exists(dir))
{
    alert("'" + dir + "'" + " doesn't exist.");
}
else
{
    var dirlist;
    dirlist = Directory.getDirectories(dir);
    
    var output;
    output += "In ";
    output += "'" + dir + "'";
    output += ", there are ";
    output += dirlist.length;
    output += " subdirectories:\n\n";
    
    for (var d in dirlist)
    {
        output += dirlist[d];
        output += "\n";
    }
    
    alert(output);
}

//(/EXAMPLE)

