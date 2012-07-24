

//(EXAMPLE)
//Description: Example 1
//Class: DbConnection
//Code:


// example 1: connecting to a database;
// TODO: enter proper values or else connection
// will take some time to timeout


var db;

// connect to the host application's local database, which
// is the project that's currently open
db = HostApp.getDatabase();

// connect to a MySQL database called 'dbname', hosted on
// mysql.my.domain at port 3306 with user name 'user' and
// password 'pw' 
db = new DbConnection("mysql://user:pw@mysql.my.domain:3306/dbname");

// connect to a SQL Server database called 'dbname', hosted on 
// mssql.my.domain at port 1433 with user name 'user' and 
// password 'pw'
db = new DbConnection("mssql://user:pw@mssql.my.domain:1433/dbname");

// connect to an Oracle database called 'dbname', hosted on 
// oracle.my.domain at port 1521 with user name 'user' and 
// password 'pw'
db = new DbConnection("oracle://user:pw@oracle.my.domain:1521/dbname");



// example 2: determining the structure of a table using 
// describeTable() and inserting the results into a table 
// using bulkInsert()


// TODO: enter input table path for which to get the schema
var g_input = "";

// TODO: enter the output table path to which to write the schema
var g_output = "";

// g_project is the database where the input table is located 
// and to which the output table will be written; here, we use 
// the application's current database
var g_project = HostApp.getDatabase();

// the execute function executes SQL statements on a database; 
// in this case, if the table already exists, delete it, then 
// create the output table with five fields: name, type, width,
// scale, and expression
g_project.execute
("
    DROP TABLE IF EXISTS " + g_output + ";

    CREATE TABLE " + g_output + " 
    (
        NAME         VARCHAR(500),
        TYPE         VARCHAR(25),
        WIDTH        NUMERIC(10,0),
        SCALE        NUMERIC(10,0),
        EXPRESSION   VARCHAR(500)
    );
");

// get the schema from the input table and
// create the inserter to insert the schema
// items
var fields = g_project.describeTable(g_input);

var inserter = g_project.bulkInsert(g_output,
                   "NAME, TYPE, WIDTH, SCALE, EXPRESSION");

for (var item in fields)
{
    // find out the type
    var nametype;
    if (fields[item].type == DbType::Character)
        nametype = "CHARACTER";
    if (fields[item].type == DbType::WideCharacter)
        nametype = "WIDECHARACTER";
    if (fields[item].type == DbType::Binary)
        nametype = "BINARY";
    if (fields[item].type == DbType::Numeric)
        nametype = "NUMERIC";
    if (fields[item].type == DbType::Double)
        nametype = "DOUBLE";
    if (fields[item].type == DbType::Integer)
        nametype = "INTEGER";
    if (fields[item].type == DbType::Date)
        nametype = "DATE";
    if (fields[item].type == DbType::DateTime)
        nametype = "DATETIME";
    if (fields[item].type == DbType::Boolean)
        nametype = "BOOLEAN";

    // fill out the inserter elements corresponding to
    // the table fields
    inserter["NAME"]       = fields[item].name;
    inserter["TYPE"]       = nametype;
    inserter["WIDTH"]      = fields[item].width;
    inserter["SCALE"]      = fields[item].scale;
    inserter["EXPRESSION"] = fields[item].expression;
    
    // insert the row
    inserter.insertRow();
}

// finalize the insert
inserter.finishInsert();

// refresh the project tree
HostApp.refresh();


//(/EXAMPLE)

