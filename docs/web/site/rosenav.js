var zone;
var zones = [
{ "v":"4", "name":"index",       "label":"home",         "url":"index.html" },
{ "v":"4", "name":"authors",     "label":"authors",      "url":"authors.html" },
{ "v":"4", "name":"history",     "label":"history",      "url":"history.html" },
{ "v":"4", "name":"development", "label":"development",  "url":"development.html" },
{ "v":"4", "name":"pictures",    "label":"pictures",     "url":"pictures.html" },
{ "v":"4", "name":"list",        "label":"mailing-list", "url":"http://lists.sourceforge.net/lists/listinfo/rosegarden-devel" },
{ "v":"2.1", "name":"index",       "label":"home",         "url":"index.html" },
{ "v":"2.1", "name":"features",    "label":"features",     "url":"features.html" },
{ "v":"2.1", "name":"faq",         "label":"faq",          "url":"faq.html" },
{ "v":"2.1", "name":"pictures",    "label":"pictures",     "url":"pictures.html" },
{ "v":"2.1", "name":"list",        "label":"mailing-list", "url":"list.html" },
];
function makeNav() {
  var url = document.location.toString();
  var v = 0;
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

    if (myv != v) {
      v = myv;
      document.write('<div class="navtitle"><a href="' + pfx + 'index.html">Rosegarden-' + v + '</a></div>');
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
