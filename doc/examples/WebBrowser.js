

//(EXAMPLE)
//Description: Example 1
//Class: WebBrowser
//Code:


// create our own class derived from form
class MyForm extends Form
{
    var m_url_textbox;
    var m_browser;
    
    function MyForm()
    {
        super("Mini Browser", 100, 100, 800, 600);
        
        m_url_textbox = new TextBox("http://www.google.com", 0, 0, 500);
        m_url_textbox.enterPressed.connect(this, onUrlEnterPressed);
        
        m_browser = new WebBrowser(0, 50, 500, 350);
        m_browser.navigate("http://www.google.com");
        
        // vertical sizer
        var main_layout = new BoxLayout(Layout.Vertical);
        main_layout.add(m_url_textbox, 0, Layout.Expand | Layout.All, 5);
        main_layout.add(m_browser, 1, Layout.Expand);
        setLayout(main_layout);
    }
    
    function onUrlEnterPressed()
    {
        // user pressed enter, navigate to the new location
        m_browser.navigate(m_url_textbox.getText());
    }
}

// create an instance of our derived MyForm class
// and show it
var f = new MyForm;
f.show();
f = null;

// start the event loop; the application will exit as
// soon as the button is clicked
Application.run();


//(/EXAMPLE)

