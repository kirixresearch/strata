

//(EXAMPLE)
//Description: Example 1
//Class: DbResult
//Code:


// connect to local project database
var db = HostApp.getDatabase();

// create a table
var result = db.execute("CREATE TABLE mytable (field1 VARCHAR(80));");

// add some rows
db.execute("
        INSERT INTO mytable (field1) VALUES ('111');
        INSERT INTO mytable (field1) VALUES ('222');
        INSERT INTO mytable (field1) VALUES ('333');
        ");

// update application's project tree (we do this
// so that the newly created table displays in the
// application project tree)
HostApp.refresh();

// select the rows and display the values
var result = db.execute("SELECT * FROM mytable");

while (result.next())
{
    alert(result.field1);
}


//(/EXAMPLE)

