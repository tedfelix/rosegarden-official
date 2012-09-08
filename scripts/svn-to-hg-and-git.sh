# Convert Rosegarden trunk to Mercurial and Git -- summary of what I
# did this time.  I think this script is probably better read &
# comprehended than simply run.

start_rev=7563

# Get a copy of fast-export (will use this for git repo later)
git pull git://repo.or.cz/fast-export.git

# Clone the SourceForge SVN history into sf-svn
mkdir sf-svn
rsync -av rosegarden.svn.sourceforge.net::svn/rosegarden/* ./sf-svn/

# Create a branchmap to place the RG "branch" on default
cat > branchmap <<EOF
rosegarden default
EOF

# Create an authormap for the authors found in this timeframe
cat > authormap <<EOF
alteholz = Thorsten Alteholz <rosegarden@alteholz.de>
alvarudras = alvar udras <alvar.udras@gmail.com>
cannam = Chris Cannam <cannam@all-day-breakfast.com>
cjnfryer = Chris Fryer <chrisf1874@googlemail.com>
dmmcintyr = D. Michael McIntyre <michael.mcintyre@rosegardenmusic.com>
emrum = Emanuel Rumpf <em.rumpf@gmail.com>
glaurent = Guillaume Laurent <glaurent@telegraph-road.org>
gperciva = Graham Percival <g.percival@elec.gla.ac.uk>
gzoumix = Michaël Lienhardt <gzoumix@users.sourceforge.net>
hannibal203 = hannibal203 <hannibal203@users.sourceforge.net>
hjunes = Heikki Junes <hjunes@gmail.com>
iangardner = Ian Gardner <ilgardner@yahoo.co.uk>
ilan1 = Ilan Tal <ilan.tal@gmail.com>
janifr = Jani Frilander <j.frilander@gmail.com>
msjulie = Julie S. <msjulie_s@yahoo.com>
nvdberg = Niek van den Berg <Niek.vandenBerg@inter.nl.net>
pjhacnau = Peter Howard <pjhacnau@users.sourceforge.net>
plcl = Pedro Lopez-Cabanillas <pedro.lopez.cabanillas@gmail.com>
prokoudine = Alexandre Prokoudine <alexandre.prokoudine@gmail.com>
raboofje = Arnout Engelen <rosegarden@bzzt.net>
tedfelix = Ted Felix <ted@tedfelix.com>
tehom = Tom Breton <tehom@panix.com>
vacuum-for-time = Shelagh Manton <shelagh.manton@gmail.com>
yguillemot = Yves Guillemot <yc.guillemot@wanadoo.fr>
EOF

# Convert trunk to a Mercurial repo
hg convert \
   --datesort \
   --branchmap branchmap \
   --authormap authormap \
   --config convert.svn.trunk=trunk/rosegarden \
   --config convert.svn.startrev="$start_rev" \
   file://`pwd`/sf-svn/trunk/rosegarden \
   hg-trunk

# Convert this to git as well
git init git-trunk
cd git-trunk
../fast-export/hg-fast-export.sh -r ../hg-trunk
