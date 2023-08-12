
// Fill the event display with info.

var fill_event_display = function(data) {
	 $("#event").empty();

	 $.each(data, function() {
		  if(this["event"]=="")
				this["event"]="&nbsp;";
        if(this["is_video"]) {
		      $("#event").append("<div class='video'>"
                               +"<div class='strip_upper'></div>"
                               +"<div class='strip_lower'></div>"                               
									    +"<a class='fancybox videobox' rel='group' href='video/"+this["id"]+"'>"
									    +"<img width=200 class='lazy' data-original='video_thumbnail_"
									    +this["id"]+"' />"
									    +"</a>"
									    +"</div>");
        } else {
		      $("#event").append("<div class='photo'>"
									    +"<a class='fancybox photobox' rel='group' href='photo?id="+this["id"]+"'>"
									    +"<img width=200 class='lazy' data-original='photo_thumbnail_"
									    +this["id"]+"' />"
									    +"</a>"
									    +"</div>");
        }
	 });
	 $(".lazy").lazyload({
		  effect: "fadeIn",
		  container: $("#event_segment")
	 });
	 $(".fancybox.photobox").on('click', function(ev) {
		 ev.preventDefault();
		 ev.stopPropagation();
		  var img=$(this).attr('href');
		  $("#single_shot img").unbind('load');
		  $("#spinner").show();
		  $("#single_shot img").bind('load', function() {
				$("#spinner").hide();
				$("#single_shot").show();
				$("#single_shot").css({'z-index': 2});
				$("#single_shot").animate({
					 opacity: 1.0
				}, 500, "linear", function() {
					 $("#slide_controls").show();
				});
				
		  });
		 $("#single_shot .align_helper").css({"display": "inline-block"});
		 $("#single_shot img").css({"display": "inline-block"});		
		 $("#single_shot video").css({"display": "none"});
		 $("#single_shot img").attr('src', img);
	 });
	$(".fancybox.videobox").on('click', function(ev) {
		console.log("Clicked on video");
		ev.preventDefault();
		ev.stopPropagation();		
		var m8u3=$(this).attr('href')+"/index.m3u8";
		console.log(m8u3);
		$("#single_shot img").unbind('load');
		$("#single_shot").show();
		$("#single_shot").css({'z-index': 2});
		$("#single_shot").animate({
			opacity: 1.0
		}, 500, "linear", function() {
		});
		$("#single_shot .align_helper").css({"display": "none"});
		$("#single_shot video").attr('poster', 'video_thumbnail_2792');
		$("#single_shot img").css({"display": "none"});		
		$("#single_shot video").css({"display": "block"});

		var video = $("#single_shot video")[0];
		console.log("video element", video);
		var videoSrc = m8u3;
		if (Hls.isSupported()) {
			console.log("Using HLS library");
			var hls = new Hls();
			hls.loadSource(videoSrc);
			hls.attachMedia(video);
		}
		else if (video.canPlayType('application/vnd.apple.mpegurl')) {
			console.log("Using HLS native");			
			video.src = videoSrc;
		}

	});
    var next_photo = function() {
		  // Find the next photo.
		  var this_photo     = $("#single_shot img").attr("src");
//		  console.log(this_photo);
		  var this_thumbnail = $(".fancybox[href='"+this_photo+"']");
//		  console.log(this_thumbnail);
		  var next_photo = $(this_thumbnail).parent().next().find("a");
//		  console.log(next_photo);
		  if(next_photo.length==1) {
				$("#single_shot img").unbind('load');
				$("#spinner").show();
				$("#single_shot img").bind('load', function() {
					 $("#spinner").hide();
				});
				$("#single_shot img").attr('src', $(next_photo).attr('href'));
		  }
	 };
    var prev_photo = function() {
		  // Find the previous photo.
		  var this_photo     = $("#single_shot img").attr("src");
//		  console.log(this_photo);
		  var this_thumbnail = $(".fancybox[href='"+this_photo+"']");
//		  console.log(this_thumbnail);
		  var prev_photo = $(this_thumbnail).parent().prev().find("a");
//		  console.log(prev_photo);
		  if(prev_photo.length==1) {
				$("#single_shot img").unbind('load');
				$("#spinner").show();
				$("#single_shot img").bind('load', function() {
					 $("#spinner").hide();
				});
				$("#single_shot img").attr('src', $(prev_photo).attr('href'));
		  }
	 };
    
    $("#button_next").on('click', next_photo);
	 $("#button_prev").on('click', prev_photo);
    $(document).on("keydown", function(event) {
        if(event.which==39) { // right
            next_photo();
        }
        if(event.which==37) { // left
            prev_photo();
        }
    });
    
	 $("#button_close").on('click', function(e) {
				$("#slide_controls").hide();
				var width=$(window).width();
				$("#single_shot").animate({ 
					 left: -width,
					 right: width,
					 opacity: 0,
				}, 500, "linear", function() {
					 $("#single_shot").hide();
					 $("#single_shot").css({'z-index': -2, left: 0, right: 0});
				});
	 });
	 $(document).keyup(function(e) {
		  if(e.keyCode == 27) {
				$("#slide_controls").hide();
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

// Load a list of all users and their access permissions for this
// event.

var load_users = function(event_id) {
	 var json={};
	 json["action"]="users";
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
				$("#spinner").hide();
				$("#event_header").html(data["name"]);
				if(data["photos"].length==0) 
					 window.location="login.html";
				else
					 fill_event_display(data["photos"]);
		  },
		  error: function (xhr, desc, err)
		  {
				$("#spinner").hide();
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
	 $("#spinner").show();
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
				$("#spinner").hide();
				$("#event_header").html(data["name"]);
				if(data["photos"].length==0) 
					 window.location="login.html";
				else
					 fill_event_display(data["photos"]);
		  },
		  error: function (xhr, desc, err)
		  {
				$("#spinner").hide();
		  }
	 });
};

// Main entry.

$(document).ready( function() {
	 var qd = {};

	 $("#spinner").hide();

	 location.search.substr(1).split("&").forEach(function(item) {var k = item.split("=")[0], v = item.split("=")[1]; v = v && decodeURIComponent(v); (k in qd) ? qd[k].push(v) : qd[k] = [v]})

    if(qd["id"])
	     load_photos(qd["id"][0]);
    else
        window.location="login.html";

    $("#access").on('click', function(e) {
        e.preventDefault();
        load_users(qd["id"][0]);
    });

    $("#back_button").on('click', function(e) {
        e.preventDefault();
        history.back();
    });
    
	 $("#fullscreen").on('click', function(e) {
		  e.preventDefault();
        if(document.fullscreenElement==null) {
		      document.getElementById("app").requestFullscreen();
        } else {
		      document.exitFullscreen();
        }
	 });
});
