

//(EXAMPLE)
//Description: Example 1
//Class: TextWriter
//Code:


var text;

// try to create a text file
var writer = File.createText("c:\\testfile.txt");
if (writer)
{
    writer.writeLine("Line 1");
    writer.writeLine("Line 2");
    writer.writeLine("Line 3");
    
    writer.write("Line 4");
    writer.write(" - still on line 4");
    writer.writeLine(" - still on line 4");
    writer.writeLine("Line 5");
}


//(/EXAMPLE)

