var zone;
var zones = [
{ "v":"4", "name":"project home", "label":"project home",  "url":"http://www.all-day-breakfast.com/rosegarden/" },
{ "v":"4", "name":"tutorial home", "label":"tutorial home",  "url":"index.html" },
{ "v":"4", "name":"using Rosegarden", "label":"Using Rosegarden","url":"using-rosegarden/toc.html" }
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
      document.write('<div class="navtitle">Rosegarden-' + lastv + '</div>');
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
      /*
      document.write('<div class="navtitle"><a href="' + pfx + 'download.html">Download</a></div>');
        */
    }
  }
}
