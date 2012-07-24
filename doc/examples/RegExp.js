

//(EXAMPLE)
//Description: Example 1
//Class: RegExp
//Code:


// sample string containing some numbers
var string = "First Number: 1234; Second Number: 5678";

// regular expression to match whole numbers
var regex = new RegExp("[0-9]+","mg");

// find each regular expression match in the string
var result;
while ((result = regex.exec(string)) != null)
{
    // if we have a match, do something
    alert(result[0]);
}


//(/EXAMPLE)

