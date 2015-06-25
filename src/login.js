$.fn.serializeObject = function()
{
   var o = {};
   var a = this.serializeArray();
   $.each(a, function() {
       if (o[this.name]) {
           if (!o[this.name].push) {
               o[this.name] = [o[this.name]];
           }
           o[this.name].push(this.value || '');
       } else {
           o[this.name] = this.value || '';
       }
   });
   return o;
};

$(document).ready( function() {

	 $(document).on("submit", "form", function(event) {
		  event.preventDefault();        
		  var mydata = JSON.stringify($('form').serializeObject());
		  $.ajax({
				url: $(this).attr("action"),
				type: "POST",
				dataType: "JSON",
				data: mydata,
				processData: false,
				contentType: "application/json",
				success: function (data, status)
				{
					 if(data["status"]=="error")
						  alert('not allowed!');
					 else 
						  alert('ok');
				},
				error: function (xhr, desc, err)
				{
					 
				}
		  });        
	 });
});
