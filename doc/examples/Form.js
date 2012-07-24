

//(EXAMPLE)
//Description: Example 1
//Class: Form
//Code:


// create our own class derived from form

class MyForm extends Form
{
    function MyForm()
    {
        super("Hello World", 100, 100, 200, 100);
        
        var b = new Button("Exit", 60, 20, 80, 24);
        this.add(b);
        b.click.connect(this, onButtonClicked);
    }
    
    function onButtonClicked()
    {
        Application.exit();
    }
}

// create an instance of our derived MyForm class
// and show it
var f = new MyForm;
f.show();

// start the event loop; the application will exit as
// soon as the button is clicked
Application.run();

alert("Application exiting...");


//(/EXAMPLE)

