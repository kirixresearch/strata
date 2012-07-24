

//(EXAMPLE)
//Description: Example 1
//Class: Extension
//Code:


// assuming an extension file with a picture called "mountains.jpg"
// and a script file called "main.js", the following should demonstrate
// loading bitmap and text resources

class MyForm extends Form
{

    function MyForm()
    {
        var bmp = Extension.getBitmapResource("mountains.jpg");
        var width = bmp.getWidth();
	    var height = bmp.getHeight();

        super("Startup Picture", 1, 1, width-20, height);
        this.center();
 
        var picture = new PictureBox(0, 0, width, height-70);
        picture.setImage(bmp);
        this.add(picture);
        
        var button = new Button("Show Source", 5, height-65, 80, 24);
        button.click.connect(this, onShowSource);
        this.add(button);
        
    }
    
    function onShowSource()
    {
        var source = Extension.getTextResource("main.js");
        alert(source);
    }
}


var f = new MyForm;
f.show();
f = null;

Application.run();


//(/EXAMPLE)

