

//(EXAMPLE)
//Description: Example 1
//Class: HostApp
//Code:


// example 1: getting a list of open documents and closing
// the documents that begin with "http"


// get all the host application's open documents, which 
// are returned by HostApp.getDocuments() as an array of 
// HostDocument objects
var g_all_documents;
g_all_documents = HostApp.getDocuments();

for (var doc in g_all_documents)
{
    // for each open document, find the location
    var location;
    location = g_all_documents[doc].getLocation();
    
    // if the location starts with http, close it; note: 
    // this doesn't close all web documents, but only
    // those that start with http; this is simply to
    // demonstrate some of the functions for working with
    // open host documents
    if (location.substr(0,4) == "http")
        HostApp.close(g_all_documents[doc]);
}



// example2: storing a list of browsed locations in a
// database table


// create a browse history object and call Application.run(), 
// which starts the event loop that processes frame events
var history = new BrowseHistory();
Application.run();


// browse history class definition
class BrowseHistory
{
    // class member variables for the local database and output table
    var m_db = HostApp.getDatabase();
    var m_table = "browse_history";

    function BrowseHistory()
    {
        // note: this is the browse history class constructor
        // and is called when a browse history object is created;
        // this constructor simply connects the host application 
        // location changed event to the onLocationChanged function
 
        // connect the onLocationChanged event handler to process
        // location changed events
        HostApp.locationChanged.connect(this, onLocationChanged);
    }

    function onLocationChanged(sender, evt)
    {
        // note: this function is called when the host application
        // location changed event is fired; when this happens, this
        // function checks if a browse history table exists, and if 
        // it doesn't, it creates one; then it adds a new row to the
        // table that contains the new location along with a timestamp 
        // indicating when the location changed

        if (!m_db.exists(m_table))
        {
            // if the browse history table doesn't exist, create it
            m_db.execute
            ("
                CREATE TABLE " + m_table + "
                (
                    TITLE          VARCHAR(200),
                    LINK           VARCHAR(200),
                    BROWSE_DATE    DATETIME(8)
                );
            ");

            // refresh the host application project tree
            HostApp.refresh();
        }

        // find out the date and time
        var dt = new Date();
        var browse_date =  dt.getFullYear() + "-" +
                          (dt.getMonth() + 1) + "-" +
                           dt.getDate() + " " +
                           dt.getHours() + ":" +
                           dt.getMinutes() + ":" +
                           dt.getSeconds();        

        // create a SQL statement to insert the new location into the
        // browse history table, along with a timestamp indicating when 
        // the location changed
        var sql = "INSERT INTO " + m_table + 
                  " (TITLE, LINK, BROWSE_DATE) VALUES ('" + 
                  evt.caption + "','" + evt.location + "','" + browse_date + "');";
 
        // execute the SQL statement to actually add the record
        // to the history
        m_db.execute(sql);
    }
}


//(/EXAMPLE)

