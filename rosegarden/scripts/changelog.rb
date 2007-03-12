#!/usr/local/bin/ruby -w

require 'getoptlong'
require 'date'

#
# A Class representing one log entry, with author, date, and message
# Objects from this class can be put into a hash
#
class LogEntry
  include Comparable

  attr_reader :author, :date, :logMessage

  def initialize headerLine, logMessage
    parseHeaderLine headerLine
    @logMessage = logMessage
  end

  def parseHeaderLine headerLine
    fields = headerLine.split
    dateArray = fields[1].split("/")
    timeArray = fields[2].split(":")
    @date = Time.gm(dateArray[0], dateArray[1], dateArray[2],
		    timeArray[0], timeArray[1], timeArray[2])
    @author = fields[4].chop
  end

  def <=>(anOther)
    @date <=> anOther.date
  end

  def eql?(anOther)
    logMessage.eql? anOther.logMessage
  end

  def to_s
    return "From #{author} at #{date} :\n" + logMessage
  end

  def hash
    return logMessage.hash
  end
end

#
# Parse cvs log from a given IO stream
#
class LogParser

  def initialize logstream
    @logstream = logstream
    @logs = Hash.new
  end

  #
  # Parse and dump all the logs
  #
  def parse
    @logstream.each do |line|
      if (line =~ /^RCS file:/)
	parseFileLog
      end
    end

    @logs.keys.sort.each { |logEntry| puts "Log for :", @logs[logEntry].sort.join(", "), "" , logEntry, "\n---------------\n" }
    #@logs.each { |logEntry, files| puts "Log for :", files.sort.join(", "), "" , logEntry, "\n---------------\n" }
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

	# Take the log in account only if it's from HEAD
	if (HeadRevNum.match(revisionNumber))
	  # Update the list of files this log entry pertains to

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
    headerLine = @logstream.readline
    headerLine.chomp!

    logMessage = ""
    line = ""
    until (line =~ /^[-=]+$/)
      logMessage += line unless line.nil? || line =~ /^branches:\s+[\d.]+;$/
      line = @logstream.readline
    end

    puts "Log Entry for #{headerLine} : #{logMessage}" if $DEBUG
    return LogEntry.new(headerLine, logMessage), (line =~ /^=+$/)
  end

end

#
# Main class - parse log from the 'cvs log' command
#
class LogExtractor
  def initialize dFrom, dTo = "", files = ""
    @from = dFrom
    @to = dTo
    @files = files
  end

  def extract
    cmd = "cvs -q -z3 log -d '#{@from}<#{@to}' #{@files}"
    puts cmd if $DEBUG
    @logStream = IO.popen cmd
    @logParser = LogParser.new @logStream
    @logParser.parse
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
