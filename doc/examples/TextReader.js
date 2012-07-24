

//(EXAMPLE)
//Description: Example 1
//Class: TextReader
//Code:


var text = "";

// try to open the text file
var reader = File.openText("c:\\testfile.txt");
if (!reader)
{
    alert("Can't open the file.  Does it exist?");
}
else
{
    // file opened successfully;
    // read in all of the lines in the file
    
    var line;
    while ((line = reader.readLine()) != null)
    {
        text += line;
        text += "\n";
    }
 
    alert(text);
}


//(/EXAMPLE)


