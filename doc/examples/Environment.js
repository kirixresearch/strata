

//(EXAMPLE)
//Description: Example 1
//Class: Environment
//Code:


var output = "";

output += "Document Path: \t";
output += Environment.getDocumentsPath();
output += "\n";

output += "System Path: \t";
output += Environment.getSystemPath();
output += "\n";

output += "Temporary Path: \t";
output += Environment.getTempPath();
output += "\n\n";

alert(output);


//(/EXAMPLE)

