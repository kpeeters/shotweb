
// Fill the event display with info.

var fill_event_display = function(data) {
	 $("#events").empty();

	 $.each(data, function() {
		  if(this["event"]=="")
				this["event"]="&nbsp;";
		  $("#events").append("<div class='event'>"
									 +"<a href='event.html?id="+this["id"]+"'>"
									 +"<img class='lazy' data-original='event_thumbnail_"
									 +this["id"]+"' />"
									 +"<div class='event_title'>"+this["event"]+"</div>"
									 +"</a>"
									 +"</div>");
	 });
	 $(".lazy").lazyload({
		  effect: "fadeIn",
		  cache: true
	 });
};

// Load a list of events and generate placeholder divs for their
// cover thumbnails and descriptions. We will load the images later.

var load_events = function() {
	 var json={};
	 json["action"]="events";
	 var mydata = JSON.stringify(json);
	 $.ajax({
		  url: "json",
		  type: "POST",
		  dataType: "JSON",
		  data: mydata,
		  processData: false,
		  contentType: "application/json",
		  success: function (data, status)
		  {
				fill_event_display(data);
		  },
		  error: function (xhr, desc, err)
		  {
		  }
	 });
};

// Main entry.

$(document).ready( function() {
	 load_events();
});
