var zone;
var zones = [
  { "name":"index",       "label":"home",         "url":"index.html" },
  { "name":"features",    "label":"features",     "url":"features.html" },
  { "name":"faq",         "label":"faq",          "url":"faq.html" },
  { "name":"pictures",    "label":"pictures",     "url":"pictures.html" },
  { "name":"list",        "label":"mailing-list", "url":"list.html" },
  { "name":"development", "label":"development",  "url":"development.html" },
  { "name":"download",    "label":"download",     "url":"download.html" }
];
function makeNav() {
  for (var i = 0; i < zones.length; ++i) {
    document.write('<div class="navlink"><span class="nav');
    if (zone) {
      if (zones[i].name != zone) document.write('un');
    } else {
      var url = document.location.toString();
      if (url.charAt(url.length - 1) == '/') url += "index.html";
      if (url.indexOf(zones[i].name + ".html") == -1) document.write('un');
    }
    document.write('selected">&#0187;</span><a href="' +
                   zones[i].url + '">' + zones[i].label + '</a></div>');
  }
}
