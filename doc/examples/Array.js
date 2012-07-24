

//(EXAMPLE)
//Description: Example 1
//Class: Array
//Code:


// creating a simple array
var arr = new Array;

arr[0] = "aaa";
arr[1] = "bbb";
arr[2] = "ccc";

for (var i in arr)
{
    alert("element " + i +" is " + arr[i]);
}


// sort an array of strings
var array_of_strings = new Array("Finch",
                                 "Bluebird",
                                 "Goose",
                                 "Sparrow",
                                 "Hummingbird",
                                 "Duck");

array_of_strings.sort();
alert(array_of_strings);


// sort an array of numbers -- this requires
// writing a sort routine of our own, because
// by default the sort() method sorts as if
// all elements were strings

function numericSort(a, b)
{
    return (a < b ? -1 : 1);
}

var array_of_numbers = new Array(5,4,10,50,45,52,1,12);

array_of_numbers.sort(numericSort);
alert(array_of_numbers);


//(/EXAMPLE)

