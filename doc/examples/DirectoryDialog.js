

//(EXAMPLE)
//Description: Example 1
//Class: DirectoryDialog
//Code:


// display a directory dialog
// and show a message box with the selected
// path if the user pressed OK

var dlg = new DirectoryDialog;
if (dlg.showDialog())
{
    alert(dlg.getPath());
}


//(/EXAMPLE)

