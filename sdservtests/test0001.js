
tests.push(function() {

	var passed = false;

    $.ajax({
                url: g_url,
				type: 'POST',
				data: {
				},
                async: false,
                success: function (data)
				{
					// do test of data here
					var obj = JSON.parse(data);
					
					if (obj.success)
						passed = true;
				},
				error: function ()
				{
					alert("AJAX error");
					passed = false;
				}
            });

    return passed; // passed
});
