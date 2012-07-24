

//(EXAMPLE)
//Description: Example 1
//Class: Timer
//Code:


// create a new instance of our timer class
// and start the application event loop
var t = new MyTimer;
Application.run();


// define our timer class
class MyTimer
{
    function MyTimer()
    {
        // initialize a member variable to track the
        // total amount of time that has elapsed once
        // the timer is started
        m_time_total = 0;

        // initialize the interval in milliseconds at 
        // which to fire the timer event
        m_time_interval = 100;

        // create a new timer object, set it to fire the
        // tick event at the specified interval, and connect
        // the tick event to the onTick member function
        m_timer = new Timer;
        m_timer.setInterval(m_time_interval);
        m_timer.tick.connect(this, onTick);
        
        // start the timer
        m_timer.start();
    }
    
    function onTick()
    {
        // ntoe: the onTick member function will be called
        // everytime the specified interval has elapsed,
        // which in this case is 100 milliseconds
    
        // track the total time since the event
        // started firing
        m_time_total += m_time_interval;
        
        // if 1 second has elapsed, stop the timer
        // and issue an alert
        if (m_time_total >= 1000)
        {
            m_timer.stop();
            alert("1 second has elapsed");
        }
    }

    // member variables
    var m_time_total;
    var m_time_interval;
    var m_timer;
}


//(/EXAMPLE)

