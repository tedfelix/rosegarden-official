var zone;
var zones = [
{ "v":"4", "name":"index",       "label":"home",         "url":"index.html" },
{ "v":"4", "name":"pictures",    "label":"screenshots",     "url":"pictures.html" },
{ "v":"4", "name":"requirements",    "label":"requirements",     "url":"requirements.html" },
{ "v":"4", "name":"documents",    "label":"documents",     "url":"documents.html" },
{ "v":"4", "name":"demonstrations",    "label":"demonstrations",     "url":"demos.html" },
{ "v":"4", "name":"history",     "label":"history",      "url":"history.html" },
{ "v":"4", "name":"development", "label":"development",  "url":"development.html" },
{ "v":"4", "name":"faq",         "label":"faq",          "url":"faq.html" },
{ "v":"4", "name":"list",        "label":"mailing-list", "url":"list.html" },
{ "v":"4", "name":"contact",         "label":"contact",          "url":"authors.html" },
{ "v":"2.1", "name":"index",       "label":"home",         "url":"index.html" },
{ "v":"2.1", "name":"features",    "label":"features",     "url":"features.html" },
{ "v":"2.1", "name":"faq",         "label":"faq",          "url":"faq.html" },
{ "v":"2.1", "name":"pictures",    "label":"screenshots",     "url":"pictures.html" },
{ "v":"2.1", "name":"list",        "label":"mailing-list", "url":"list.html" }
];
function makeNav() {
  var url = document.location.toString();
  var lastv = 0;
  for (var i = 0; i < zones.length; ++i) {

    var myv = zones[i].v;
    var inv = true;
    if (myv == "2.1") inv = (url.indexOf(myv) != -1);
    else inv = (url.indexOf("2.1") == -1);

    var pfx = "";
    if (!inv) {
      if (myv == "4") pfx = '../';
      else pfx = '2.1/';
    }

    if (myv != lastv) {
      lastv = myv;
      document.write('<div class="navtitle"><a href="' + pfx + 'index.html">Rosegarden-' + lastv + '</a></div>');
    }

    if (inv) {
      document.write('<div class="navlink"><span class="nav');
      if (zone) {
        if (zones[i].name != zone) document.write('un');
      } else {
        if (url.charAt(url.length - 1) == '/') url += "index.html";
        if (url.indexOf(zones[i].name + ".html") == -1) document.write('un');
      }

      var target = zones[i].url;
      document.write('selected">&#0187;</span><a href="' +
                     target + '">' + zones[i].label + '</a></div>');
    } 

    if (i == zones.length - 1) {
      if (url.indexOf("2.1") != -1) pfx = "../";
      else pfx = "";
      document.write('<div class="navtitle"><a href="' +
	pfx + 'download.html">Download</a></div>');
    }
  }
}
