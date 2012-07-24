

//(EXAMPLE)
//Description: Example 1
//Class: Button
//Code:


// create an instance of our derived MyForm class
var f = new MyForm;
f.show();
Application.run();

// create our own class derived from Form
class MyForm extends Form
{
    function MyForm()
    {
        // call the constructor on the base class Form
        super("Button Example", 100, 100, 200, 100);
        
        // create a button and add the button to our form
        var button = new Button("Exit", 60, 20, 80, 24);
        this.add(button);
        
        // when the button is clicked, call the onButtonClicked
        // event handler on this class
        button.click.connect(this, onButtonClicked);
    }
    
    function onButtonClicked()
    {
        // when the button is clicked, exit the application
        Application.exit();
    }
}


//(/EXAMPLE)

