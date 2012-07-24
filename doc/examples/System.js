

//(EXAMPLE)
//Description: Example 1
//Class: System
//Code:


// sleep (pause) for two and a half seconds
System.sleep(2500);

// now measure the time it takes for the user to press ok
var clock1, clock2;
clock1 = System.clock();
alert("Press Ok.");
clock2 = System.clock();
alert("It took " + (clock2-clock1)/System.getClocksPerSecond() +
                  " seconds to click Ok.");

// now launch notepad
System.execute("notepad");


//(/EXAMPLE)

