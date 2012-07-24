

//(EXAMPLE)
//Description: Example 1
//Class: TextBox
//Code:


// create our own class derived from form
class MyForm extends Form
{
    var m_text1;
    var m_text2;
    
    function MyForm()
    {
        super("Simple Entry", 100, 100, 200, 150);
        
        // create a text control
        m_text1 = new TextBox("", 5, 5, 180, 24);
        this.add(m_text1);
        
        // in m_text1, handle the event that fires when
        // the text is changed, so we can mirror the text
        // in m_text2 (this demonstrates the textChanged event)
        m_text1.textChanged.connect(this, onTextChanged);
       
        // create another text control
        m_text2 = new TextBox("", 5, 35, 180, 24);
        this.add(m_text2);
 
        var b = new Button("Exit", 60, 75, 80, 24);
        this.add(b);
        b.click.connect(this, onButtonClicked);
    }
    
    function onTextChanged(sender, params)
    {
        // set the text in the second control to the text
        // in the first control that has been changed
        m_text2.setText(params.text);
    }
    
    function onButtonClicked()
    {
        // exit the event loop
        Application.exit();
    }
}

// create an instance of our derived MyForm class
// and show it
var f = new MyForm;
f.show();
f = null;

// start the event loop
Application.run();


//(/EXAMPLE)

