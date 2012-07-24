

//(EXAMPLE)
//Description: Example 1
//Class: DialogResult
//Code:


// create an alert message with Yes and No buttons
alert("Are you sure you would like to continue?", "Title", DialogResult.YesNo);

// create an alert message with OK and Cancel buttons
alert("Message", "Title", DialogResult.Ok | DialogResult.Cancel);

// create an alert message and only execute the code if the Yes button is pressed
if (alert("Are you sure you would like to continue?", "Title", DialogResult.YesNo) == DialogResult.Yes)
{
    // code to execute if the Yes button is pressed
    alert("Yes was clicked!");
}


//(/EXAMPLE)

