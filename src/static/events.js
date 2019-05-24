
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
    var monthNames = [
        "Jan", "Feb", "Mar", "Apr", "May", "Jun", "Jul", "Aug", "Sep", "Oct", "Nov", "Dec"
    ];

	 $.each(data, function() {
		  if(this["event"]=="")
				this["event"]="&nbsp;";
        var date=new Date(this["start"]*1000);
		  $("#events").append("<div class='event'>"
									 +"<a href='event.html?id="+this["id"]+"'>"
									 +"<img class='lazy' data-src='event_thumbnail_"
									 +this["id"]+"' /></a>"
									 +"<div class='event_controls'>"
                            +"<div class='date'>"+monthNames[date.getMonth()]+" "+date.getFullYear()+"</div>"
                            +"<div class='event_title'>"
                            +"<a href='event.html?id="+this["id"]+"'>"+this["event"]+"</a></div><a class='share_event' href=''><i id='share_event_"+this["id"]+"' class='material-icons'>share</i></a><a class='person_add' href=''><i class='material-icons'>person_add</i></a></div>"
									 +"</a>"
									 +"</div>");
	 });
    $("img.lazy").lazyload();
    $(".share_event").on('click', function(e) {
        e.preventDefault();
	     var json={};
	     json["action"]="share";
        json["event_id"]=parseInt(e.target.id.replace("share_event_", ""));
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
					     window.location="login.html";
                $("#dialog_link").html("<a href='"+data["url"]+"'>"+data["url"]+"</a>");
                $("#events_dialog").css({'visibility': 'visible'});
		      },
		      error: function (xhr, desc, err)
		      {
		      }
	     });
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
					 window.location="login.html";
				fill_event_display(data);
		  },
		  error: function (xhr, desc, err)
		  {
		  }
	 });
};

// Get the account information.

var load_account_info = function() {
    var json={};
    json["action"]="account";
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
					 window.location="login.html";
            if(data["allowed_add_user"]) {
                $("#admin").css({"visibility": "visible"});
                $("#access").css({"visibility": "visible"});
            }
            if(data["allowed_download"])
                $("#download").css({"visibility": "visible"});
            if(data["allowed_share"])
                $("#share").css({"visibility": "visible"});
		  },
		  error: function (xhr, desc, err)
		  {
		  }
	 });
};


// Main entry.

$(document).ready( function() {
    load_account_info();
	 load_events();

    $("#logout").on('click', function(e) {
        e.preventDefault();
        deleteCookie("token");
        window.location="login.html";
    });

    $("#ok").on('click', function(e) {
        $("#events_dialog").css({'visibility': 'hidden'});
    });
    
	 $("#fullscreen").on('click', function(e) {
		  e.preventDefault();
		  // http://www.sitepoint.com/html5-full-screen-api/
		  document.getElementById("html").webkitRequestFullScreen();
		  $("#fullscreen").hide();
	 });
});
