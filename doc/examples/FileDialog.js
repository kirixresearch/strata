

//(EXAMPLE)
//Description: Example 1
//Class: FileDialog
//Code:


// create a new file dialog object
var dlg = new FileDialog;

// show these file types in the file dialogs filter
dlg.setFilter("Bitmap Files|*.bmp|JPEG Images|*.jpg;*.jpeg");

// show the dialog, and accept the results only if the
// user presses Ok

if (dlg.showDialog() == DialogResult.Ok)
{
    var files = dlg.getPaths();

    for (var f in files)
        alert(files[f]);
}


//(/EXAMPLE)

