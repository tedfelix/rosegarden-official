#!/usr/bin/ruby -w

$RG_DIR = "/home/glaurent/rosegarden"

#
# Check for rogue sequencer processes left over after a GUI crash
#
def check_stray_sequencer

  processes = `ps -auxww`.grep(/rosegarden/)

  if (processes.length == 1)
    # Check if the lone process is the sequencer
    pid, processName = processes[0].split.indexes(1, 10)

    if processName =~ /sequencer/
      puts "Found stray sequencer running, pid #{pid} - terminating"
      Process::kill "SIGTERM", pid.to_i
    end
  end

end

#
# Check for core files older than the main exec
#
def check_core_files
  begin

    rgMainExecMTime = File.stat("gui/rosegarden").mtime

    coreFilesToDelete = Dir["gui/core.*"].find_all { |x| File.stat(x).mtime < rgMainExecMTime }
    
    coreFilesToDelete.each do |file|
      puts "Found obsolete core file to delete : #{file}"
      File.delete file
    end

  rescue Exception
    # No exec, no big deal. We don't care.
  end
end

### And now for something completely different...

while 1

  Dir.chdir $RG_DIR

  check_stray_sequencer
  check_core_files

  sleep 5

end
