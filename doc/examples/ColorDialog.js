

//(EXAMPLE)
//Description: Example 1
//Class: ColorDialog
//Code:


// create a new color dialog that shows the extended
// color selector
var dlg = new ColorDialog;
dlg.showAllColors();

// add some custom colors to the dialog
dlg.setCustomColor(0, new Color(200, 0, 0)); // custom red
dlg.setCustomColor(1, 0, 0, 200);            // custom blue

// show the dialog, and if the user selects a color,
// display a message that shows the RGB values of
// the selected color

if (dlg.showDialog() == DialogResult.Ok)
{
    // the getColor() funtion returns a Color object,
    // which has red, green, and blue members
    var c = dlg.getColor();
    
    // extract the red, green, and blue values
    // from the color object and display them
    // with an alert
    var output = "";
    output += "Red: " + c.red + "\n";
    output += "Green: " + c.green + "\n";
    output += "Blue: " + c.blue + "\n";
    alert(output);
}


//(/EXAMPLE)

