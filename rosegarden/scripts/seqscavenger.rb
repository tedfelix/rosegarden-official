#!/usr/bin/ruby -w

$RG_DIR = "/home/glaurent/rosegarden"

#
# Check for rogue sequencer processes left over after a GUI crash
#
def check_stray_sequencer

  processes = `ps alxww`.grep(/rosegarden/)

  if (processes.length == 1)
    # Check if the lone process is the sequencer
    pid, ppid, processName = processes[0].split.values_at(2, 3, 12)
    #puts "processName : #{processName} - pid : #{pid} - ppid #{ppid}"

    if processName =~ /sequencer/ && ppid.to_i == 1
      puts "Found stray sequencer running, pid #{pid} - terminating"
      Process::kill "SIGTERM", pid.to_i
    end
  end

end

#
# Check for core files older than the main exec
#
def check_core_files (path, execname)
  begin

    rgMainExecMTime = File.stat(path + "/" + execname).mtime

    # Remove core files older than binary
    obsoleteCoreFiles = (Dir[path + "/core.*"].sort{ |a,b| File.stat(a).mtime <=> File.stat(b).mtime })

    mostRecentCoreFile = obsoleteCoreFiles.pop

    if (File.stat(mostRecentCoreFile).mtime <  rgMainExecMTime)
      # it's obsolete too, so put it back in obsoleteCoreFiles
      obsoleteCoreFiles << mostRecentCoreFile
    end

#     obsoleteCoreFiles = Dir["gui/core.*"].find_all { |x| File.stat(x).mtime < rgMainExecMTime }
    
    obsoleteCoreFiles.each do |file|
      puts "Found obsolete core file to delete : #{file}"
      File.delete file
    end

    # Remove all but most recent core file

  rescue Exception
    # No exec, no big deal. We don't care.
  end
end

### And now for something completely different...

while true

  Dir.chdir $RG_DIR

  check_stray_sequencer
  check_core_files "gui", "rosegarden"
  check_core_files "sequencer", "rosegardensequencer"

  sleep 5

end
