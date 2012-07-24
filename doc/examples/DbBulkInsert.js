

//(EXAMPLE)
//Description: Example 1
//Class: DbBulkInsert
//Code:


// create a database object which points to the application
// current database

var db = HostApp.getDatabase();


// create a new table with a SQL statement

db.execute("DROP TABLE IF EXISTS OUTPUT_TABLE");
db.execute("CREATE TABLE OUTPUT_TABLE (
                    TEST_STR VARCHAR(60),
                    TEST_NUM NUMERIC(5));");


// create the inserter
var inserter = db.bulkInsert("OUTPUT_TABLE", "TEST_STR, TEST_NUM");

for (var i = 1; i <= 1000; ++i)
{
    inserter["TEST_STR"] = "ROW " + i;
    inserter["TEST_NUM"] = i;
    
    // insert the row
    inserter.insertRow();  
}

// finalize the insert
inserter.finishInsert();

// make sure the application refreshes its user
// interface so that the table can be seen

HostApp.refresh();


//(/EXAMPLE)

