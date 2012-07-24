

//(EXAMPLE)
//Description: Example 1
//Class: ComboBox
//Code:


// create a new combobox sample form
var f = new MyForm("ComboBox Example", 0, 0, 400, 300);
f.center();
f.show();
Application.run();


// define the combobox sample form
class MyForm extends Form
{
    var m_combo;
    var m_line;
    var m_textbox;
    var m_layout;

    function MyForm(caption, x_pos, y_pos, width, height)
    {
        // MyForm() gets called when a new object is created;
        // the super() function calls the contructor on the
        // base class, which in this case is Form since MyForm
        // extends Form
        super(caption, x_pos, y_pos, width, height);
    
        // create a new combobox; add some items to the
        // combobox and select the first item in the list
        m_combo = new ComboBox;
        m_combo.add("Finch");
        m_combo.add("Bluebird");
        m_combo.add("Goose");
        m_combo.add("Sparrow");
        m_combo.add("Hummingbird");
        m_combo.add("Duck");
        m_combo.select(0);
        
        // create a new line and textbox
        m_line = new Line(Line.Horizontal);
        m_textbox = new TextBox();
        m_textbox.setMultiline(true);

        // connect the combobox events to the event handler; these 
        // handlers will be called when the corresponding combo event 
        // is fired
        m_combo.enterPressed.connect(this, onEnterPressed);
        m_combo.selectionChanged.connect(this, onSelectionChanged);
        m_combo.textChanged.connect(this, onTextChanged);
        
        // connect the form resize event
        this.sizeChanged.connect(this, onFormSizeChanged);

        // add the combobox, line, and the textbox to the form
        m_layout = new BoxLayout(Layout.Vertical);
        m_layout.addSpacer();
        m_layout.add(m_combo, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer(10);
        m_layout.add(m_line, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer(10);
        m_layout.add(m_textbox, 1, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer();
        
        // use m_layout for the main form layout
        setLayout(m_layout);
    }

    function onEnterPressed(sender, event_args)
    {
        m_textbox.appendText("Enter pressed. Text: " + event_args.text + "\n");
    }

    function onSelectionChanged(sender, event_args)
    {
        m_textbox.appendText("Selection changed. Text: " + event_args.text + "\n");
    }
    
    function onTextChanged(sender, event_args)
    {
        m_textbox.appendText("Text changed. Text: " + event_args.text + "\n");
    }    
    
    function onFormSizeChanged(sender, event_args)
    {
        // when the form is resized, append text to the textbox
        // with the new form size
        var size = "(" + getSize().width + "," + getSize().height + ")";
        m_textbox.appendText("Form resized: " + size + "\n");
        layout();        
    }    
}


//(/EXAMPLE)

