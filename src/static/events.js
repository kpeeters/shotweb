
function setCookie(key, value, expireDays, expireHours, expireMinutes, expireSeconds) {
    var expireDate = new Date();
    if (expireDays) {
        expireDate.setDate(expireDate.getDate() + expireDays);
    }
    if (expireHours) {
        expireDate.setHours(expireDate.getHours() + expireHours);
    }
    if (expireMinutes) {
        expireDate.setMinutes(expireDate.getMinutes() + expireMinutes);
    }
    if (expireSeconds) {
        expireDate.setSeconds(expireDate.getSeconds() + expireSeconds);
    }
    document.cookie = key +"="+ escape(value) +
        ";domain="+ window.location.hostname +
        ";path=/"+
        ";expires="+expireDate.toUTCString();
}

function deleteCookie(name) {
    setCookie(name, "", null , null , null, 1);
}

// Fill the event display with info.

var fill_event_display = function(data) {
	 $("#events").empty();

	 $.each(data, function() {
		  if(this["event"]=="")
				this["event"]="&nbsp;";
		  $("#events").append("<div class='event'>"
									 +"<a href='event.html?id="+this["id"]+"'>"
									 +"<img class='lazy' data-original='event_thumbnail_"
									 +this["id"]+"' /></a>"
									 +"<div class='event_controls'><div class='event_title'>"
                            +"<a href='event.html?id="+this["id"]+"'>"+this["event"]+"</a></div><i class='material-icons'>share</i><i class='material-icons'>person_add</i></div>"
									 +"</a>"
									 +"</div>");
	 });
	 $(".lazy").lazyload({
		  container: $("#events_segment"),
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
				if(data.length==0)
					 window.location="/";
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

    $("#logout").on('click', function(e) {
        e.preventDefault();
        deleteCookie("token");
        window.location="/";
    });
    
	 $("#fullscreen").on('click', function(e) {
		  e.preventDefault();
		  // http://www.sitepoint.com/html5-full-screen-api/
		  document.getElementById("html").webkitRequestFullScreen();
		  $("#fullscreen").hide();
	 });
});
