#!/usr/local/bin/ruby -w

require 'getoptlong'
require 'date'

class LogExtractor
  def initialize dFrom, dTo = "", files = ""
    @from = dFrom
    @to = dTo
    @files = files
  end

  def extract
    cmd = "cvs -q -z3 log -r -d '#{@from}<#{@to}' #{@files}"
    puts cmd if $DEBUG
    @logStream = IO.popen cmd
    @logParser = LogParser.new @logStream
    @logParser.parse
  end
end

class LogParser

  def initialize logstream
    @logstream = logstream
    @logs = Hash.new
  end

  def parse
    @logstream.each do |line|
      if (line =~ /^RCS file:/)
	parseFileLog
      end
    end

    @logs.each { |key, value| puts "Log for :", value.sort.join(", "), "" , key, "\n---------------\n" }
  end

  HeadRevNum = Regexp.new('^\d+\.\d+$')

  def parseFileLog

    workingFile = @logstream.readline
    workingFile.slice!(/^Working file: /)
    workingFile.chomp!

    nbLogs = 0
    @logstream.each do |line|

      # Beginning of log entry is 'revision x.y'
      if (m = /^revision ([\d.]+)$/.match(line) )
	revisionNumber = m[1]
	logEntry, lastLog = parseLogEntry

	# Take the long in account only if it's from HEAD
	if (HeadRevNum.match(revisionNumber))
	  files = @logs[logEntry] || []
	  files << workingFile unless files.include? workingFile
	  @logs[logEntry] = files
	  nbLogs += 1
	else
	  puts "Skipping log for branch #{revisionNumber}" if $DEBUG
	end
      end

      # End of file log is '============'
      break if (line =~ /^=+$/) || lastLog
    end

    if (nbLogs > 0)
      print "#{workingFile} : #{nbLogs} change"
      print "s" if (nbLogs > 1)
      print "\n"
    else
      puts "#{workingFile} : no change" if $DEBUG
    end
  end

  def parseLogEntry
    # read date
    date = @logstream.readline
    date.chomp!

    logEntry = ""
    line = ""
    until (line =~ /^[-=]+$/)
      logEntry += line unless line.nil? || line =~ /^branches:\s+[\d.]+;$/
      line = @logstream.readline
    end

    puts "Log Entry for #{date} : #{logEntry}" if $DEBUG
    return logEntry, (line =~ /^=+$/)
  end

end

def usage
  STDERR << File.basename($0) <<
    " [ --from | -f ] yyyy-mm-dd " <<
    " [ --to | -t ] yyyy-mm-dd " <<
    " [ --days | -d ] nb_days_ago " <<
    " [files]\n" <<
    "Shows all logs on given files (or all in current dir if none specified), in the given range.\n" <<
    "Defaults to changes since yesterday."
  exit
end
########## And now for something completely different

# logparser = LogParser.new(File.new ARGV[0])
# logparser.parse
# exit

timeFormat = /[\d]{4}-[\d]{2}-[\d]{2}/
yesterday = Date.today - 1

fromDate = yesterday.to_s
toDate = nil

opts = GetoptLong.new(
		      [ "--from", "-f", GetoptLong::REQUIRED_ARGUMENT ],
		      [ "--to",   "-t", GetoptLong::REQUIRED_ARGUMENT ],
		      [ "--days", "-d", GetoptLong::REQUIRED_ARGUMENT ],
		      [ "--help", "-h", GetoptLong::NO_ARGUMENT ]
		      )

opts.each do |opt, arg|
  if (opt == "--from")
    fromDate = arg
  elsif (opt == "--to")
    toDate = arg
  elsif (opt == "--days")
    fromDate = (Date.today - arg.to_i).to_s
  elsif (opt == "--help")
    usage
  end
end

usage unless (fromDate =~ timeFormat) &&
  ((toDate =~ timeFormat) || toDate.nil?)

logExtractor = LogExtractor.new fromDate, toDate, (ARGV[0] || "")
logExtractor.extract
