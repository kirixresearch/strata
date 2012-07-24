

//(EXAMPLE)
//Description: Example 1
//Class: Slider
//Code:


// create a new slider sample form
var f = new MyForm("Slider Example", 0, 0, 400, 300);
f.center();
f.show();
Application.run();


// define the slider sample form
class MyForm extends Form
{
    var m_slider;
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
    
        // create a new slider, with a range from 0 to 200 
        // units and an initial value of 100
        m_slider = new Slider;
        m_slider.setRange(0, 200);
        m_slider.setValue(100);
        
        // create a new line and textbox
        m_line = new Line(Line.Horizontal);
        m_textbox = new TextBox();
        m_textbox.setMultiline(true);

        // connect the slider events to the slider handler; these 
        // handlers will be called when the corresponding slider event 
        // is fired; here, we use one handler, onSliderChanged, for 
        // simplicity, but we could also add different handlers 
        // different events
        m_slider.scroll.connect(this, onSliderChanged);
        m_slider.scrollTop.connect(this, onSliderChanged);
        m_slider.scrollBottom.connect(this, onSliderChanged);
        m_slider.scrollUp.connect(this, onSliderChanged);
        m_slider.scrollDown.connect(this, onSliderChanged);
        m_slider.scrollPageUp.connect(this, onSliderChanged);
        m_slider.scrollPageDown.connect(this, onSliderChanged);
        m_slider.track.connect(this, onSliderChanged);
        m_slider.trackRelease.connect(this, onSliderChanged);
        
        // connect the form resize event
        this.sizeChanged.connect(this, onFormSizeChanged);

        // add the slider, line, and the textbox to the form
        m_layout = new BoxLayout(Layout.Vertical);
        m_layout.addSpacer();
        m_layout.add(m_slider, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer(10);
        m_layout.add(m_line, 0, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer(10);
        m_layout.add(m_textbox, 1, Layout.Expand | Layout.Left | Layout.Right, 5);
        m_layout.addSpacer();
        
        // use m_layout for the main form layout
        setLayout(m_layout);
    }

    function onSliderChanged(sender, event_args)
    {
        // when the slider is changed, append text to the textbox 
        // with the new slider value
        m_textbox.appendText("Slider changed: " + event_args.value + "\n");
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

