
// Fill the event display with info.

var fill_event_display = function(data) {
	 $("#event").empty();

	 $.each(data, function() {
		  if(this["event"]=="")
				this["event"]="&nbsp;";
		  $("#event").append("<div class='photo'>"
									 +"<a class='fancybox' rel='group' href='photo.jpg?id="+this["id"]+"'>"
									 +"<img width=200 class='lazy' data-original='photo_thumbnail_"
									 +this["id"]+"' />"
									 +"</a>"
									 +"</div>");
	 });
	 $(".lazy").lazyload({
		  container: $("#app"),
		  effect: "fadeIn"
	 });
	 $(".fancybox").fancybox( {
		  helpers : {
				overlay : {
					 css : {
						  'background' : 'rgba(100,100,100,0.5)'
					 }
				}
		  }
    });
};

// Load a list of photos in an event and generate placeholder divs for
// their thumbnails and descriptions. We will load the images later.

var load_photos = function(event_id) {
	 var json={};
	 json["action"]="photos";
	 json["id"]=event_id;
	 var mydata = JSON.stringify(json);
	 $.ajax({
		  url: "json",
		  type: "POST",
		  dataType: "JSON",
		  cache: true,
		  data: mydata,
		  processData: false,
		  contentType: "application/json",
		  success: function (data, status)
		  {
				$("#event_header").html(data["name"]);
				fill_event_display(data["photos"]);
		  },
		  error: function (xhr, desc, err)
		  {
		  }
	 });
};

// Main entry.

$(document).ready( function() {
	 var qd = {};
	 location.search.substr(1).split("&").forEach(function(item) {var k = item.split("=")[0], v = item.split("=")[1]; v = v && decodeURIComponent(v); (k in qd) ? qd[k].push(v) : qd[k] = [v]})

	 load_photos(qd["id"][0]);

	 $("#fullscreen").on('click', function(e) {
		  e.preventDefault();
		  // http://www.sitepoint.com/html5-full-screen-api/
		  document.getElementById("app").webkitRequestFullScreen();
		  $("#fullscreen").hide();
	 });
});
