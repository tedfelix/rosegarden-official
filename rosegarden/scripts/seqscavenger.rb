#!/usr/bin/ruby -w

while 1

  processes = `ps -auxww`.grep(/rosegarden/)

  if (processes.length == 1)
    # Check if the lone process is the sequencer
    pid, processName = processes[0].split.indexes(1, 10)

    if processName =~ /sequencer/
      puts "Found stray sequencer running, pid #{pid} - terminating"
      Process::kill "SIGTERM", pid.to_i
    end
  end

  sleep 10

end
