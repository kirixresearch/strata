

//(EXAMPLE)
//Description: Example 1
//Class: ProgressBar
//Code:


// create a new progress bar sample form
var f = new MyForm("ProgressBar Example", 0, 0, 400, 110);
f.center();
f.show();
Application.run();


// define the progressbar sample form
class MyForm extends Form
{
    // progress bar member variable
    var m_progressbar;

    function MyForm(caption, x_pos, y_pos, width, height)
    {
        // MyForm() gets called when a new object is created;
        // the super() function calls the contructor on the
        // base class, which in this case is Form since MyForm
        // extends Form
        super(caption, x_pos, y_pos, width, height);

        // create a progress bar and set the member variable
        // to the newly created progress bar
        m_progressbar = new ProgressBar(0,0,0,20);
        m_progressbar.setValue(0);
        m_progressbar.setMaximum(100);        

        // create a start button and place it in a button
        // sizer so it's centered in the form
        var start_button = new Button("Start");
        var layout_button = new BoxLayout(Layout.Horizontal);
        layout_button.addStretchSpacer();
        layout_button.add(start_button, 0, Layout.Expand | Layout.Top | Layout.Bottom, 0);
        layout_button.addStretchSpacer();

        // add the progress bar and the start button to 
        // the form
        var layout_main = new BoxLayout(Layout.Vertical);
        layout_main.addStretchSpacer();
        layout_main.add(m_progressbar, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        layout_main.addStretchSpacer();
        layout_main.add(layout_button, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        layout_main.addSpacer(10);
        
        // use layout_main for the main form layout
        setLayout(layout_main);
        
        // connect the start button to the event hander
        start_button.click.connect(this, onStart);
    }
    
    function onStart(sender, event_args)
    {
        // when the start button is pressed, advance the 
        // progress bar to its maximum value
        var max = m_progressbar.getMaximum();
        for (var i = 0; i < max; ++i)
        {
            m_progressbar.setValue(i);
            System.sleep(10);
        }
    }
}


//(/EXAMPLE)

