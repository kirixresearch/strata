

//(EXAMPLE)
//Description: Example 1
//Class: Graphics
//Code:


// create a new graphics sample form
var f = new MyForm("Graphics Example", 0, 0, 300, 300);
f.center();
f.show();
Application.run();


// create a custom control that displays a smile
class MyControl extends Control
{
    function MyControl(x_pos, y_pos, width, height)
    {
        // call the Control constructor and connect
        // the paint event handler
        super(x_pos, y_pos, width, height);
        this.paint.connect(this, onPaintCalled);
    }
    
    function onPaintCalled(sender, evt)
    {   
        // when the paint event handler is called, get the
        // graphics object associated with the control and
        // draw a smile
        
        // calculate some positions and dimensions based
        // on the size of the control
        var size = this.getSize();
        var center_x = size.width/2;
        var center_y = size.height/2;
        var radius = Math.min(size.width, size.height)/2 - 2;
        
        // get the graphics object associated with the control,
        // set the color of the drawing pen to blue, and draw
        // a smile
        var gr = evt.graphics;
        gr.setPen(new Pen(new Color(0, 0, 255)));
        gr.drawCircle(center_x, center_y, radius);
        gr.drawCircle(center_x - radius/3, center_y - radius/3, radius/10);
        gr.drawCircle(center_x + radius/3, center_y - radius/3, radius/10);
        
        var x = center_x - radius/3;
        var y = center_y + radius/4;
        var width = (radius*2)/3;
        var height = radius/3;
        gr.drawEllipticArc(x, y, width, height, -160, -20);
    }
}

// define the graphics sample form
class MyForm extends Form
{
    function MyForm(caption, x_pos, y_pos, width, height)
    {
        super(caption, x_pos, y_pos, width, height);
        
        // add the smile control to the form
        var client_size = this.getClientSize();
        this.add(new MyControl(0, 0, client_size.width, client_size.height));
    }
}


//(/EXAMPLE)

