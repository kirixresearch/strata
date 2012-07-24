

//(EXAMPLE)
//Description: Example 1
//Class: CheckBox
//Code:


// create a new checkbox sample form
var f = new MyForm("CheckBox Example", 0, 0, 400, 300);
f.center();
f.show();
Application.run();


// define the checkbox sample form
class MyForm extends Form
{
    var m_textbox;

    function MyForm(caption, x_pos, y_pos, width, height)
    {
        // MyForm() gets called when a new object is created;
        // the super() function calls the contructor on the
        // base class, which in this case is Form since MyForm
        // extends Form
        super(caption, x_pos, y_pos, width, height);
    
        // create checkboxes with various color labels
        var checkbox1 = new CheckBox("Red");
        var checkbox2 = new CheckBox("Yellow");
        var checkbox3 = new CheckBox("Green");
        var checkbox4 = new CheckBox("Blue");
        
        // create a new line
        var line = new Line(Line.Horizontal);

        // create a new textbox; set the member variable
        // m_textbox to the newly created textbox so we
        // can output results to the textbox in the event
        // handler
        m_textbox = new TextBox();
        m_textbox.setMultiline(true);

        // add the checkboxes to the top part of the form;
        // use a layout object to space the elements horizontally
        var layout_top = new BoxLayout(Layout.Horizontal);
        layout_top.addSpacer();
        layout_top.add(checkbox1, 0, Layout.Expand | Layout.Top | Layout.Bottom, 5);
        layout_top.addSpacer(10);
        layout_top.add(checkbox2, 0, Layout.Expand | Layout.Top | Layout.Bottom, 5);
        layout_top.addSpacer(10);
        layout_top.add(checkbox3, 0, Layout.Expand | Layout.Top | Layout.Bottom, 5);
        layout_top.addSpacer(10);        
        layout_top.add(checkbox4, 0, Layout.Expand | Layout.Top | Layout.Bottom, 5);      

        // add the checkboxes, line, and the textbox to the form
        var layout_main = new BoxLayout(Layout.Vertical);
        layout_main.addSpacer();
        layout_main.add(layout_top, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        layout_main.addSpacer(10);
        layout_main.add(line, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        layout_main.addSpacer(10);
        layout_main.add(m_textbox, 1, Layout.Expand | Layout.Left | Layout.Right, 5);
        layout_main.addSpacer();
        
        // use layout_main for the main form layout
        setLayout(layout_main);
        
        // connect the checkbox events to the event handler
        checkbox1.click.connect(this, onCheckBoxClicked);
        checkbox2.click.connect(this, onCheckBoxClicked);
        checkbox3.click.connect(this, onCheckBoxClicked);
        checkbox4.click.connect(this, onCheckBoxClicked);
    }

    function onCheckBoxClicked(sender, event_args)
    {
        // when a checkbox is clicked, output the label and
        // check status to the textbox
        if (sender.getValue())
            m_textbox.appendText(sender.getLabel() + " checked.\n");
        else
            m_textbox.appendText(sender.getLabel() + " unchecked.\n");
    } 
}


//(/EXAMPLE)

