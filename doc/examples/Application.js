

//(EXAMPLE)
//Description: Example 1
//Class: Application
//Code:


// create an instance of our derived MyForm class
// and show it
var f = new MyForm;
f.show();

// start the event loop.  At this point in the script, execution
// is turned over to the event loop, which the Application class manages.
// Execution will proceed from here only after Application.exit is called.
Application.run();

alert("This message appears after the event loop has exited.");



// our Form-derived class
class MyForm extends Form
{
    function MyForm()
    {
        super("Hello World", 100, 100, 200, 100);
        this.add(new Label("Click on the form to stop the application.",
                           5,5,100,100));
        this.mouseLeftUp.connect(exitFunction);
    }
}

function exitFunction()
{
    // calling Application.exit() will terminate the event loop.  Execution
    // will continue in the same scope in which Application.run() was called
    Application.exit();
}


//(/EXAMPLE)

