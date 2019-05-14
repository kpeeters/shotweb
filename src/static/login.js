
// Serialise a FORM into a JSON object.

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


// Main entry.

$(document).ready( function() {

	 $(document).on("submit", "form", function(event) {
		  event.preventDefault();     
		  // Convert the form to a key-value object and add the action.
		  var formobj = $('form').serializeObject();
		  formobj["action"]="login";
		  var mydata = JSON.stringify(formobj);
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
					 else {
						  $("#login_segment").hide();
						  window.location.href="events.html";
					 }
				},
				error: function (xhr, desc, err)
				{
					 
				}
		  });        
	 });
});
