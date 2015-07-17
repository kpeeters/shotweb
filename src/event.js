
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
		  effect: "fadeIn",
		  container: $("#event_segment")
	 });
	 $(".fancybox").on('click', function(ev) {
		  ev.preventDefault();
		  var next_photo = $(this).parent().next().find("a");
		  console.log(next_photo);
		  var img=$(this).attr('href');
		  $("#singel_shot img").unbind('load');
		  $("#single_shot img").bind('load', function() {
				$("#single_shot").show();
				$("#single_shot").css({'z-index': 2});
				$("#single_shot").animate({
					 opacity: 1.0
					 }, 500, "linear");
				
		  });
		  $("#single_shot img").attr('src', img);
	 });
	 $(document).keyup(function(e) {
		  if(e.keyCode == 27) {
				var width=$(window).width();
				$("#single_shot").animate({ 
					 left: -width,
					 right: width,
					 opacity: 0,
				}, 500, "linear", function() {
					 $("#single_shot").hide();
					 $("#single_shot").css({'z-index': -2, left: 0, right: 0});
				});
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
