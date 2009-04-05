#!/usr/bin/ruby -w

`ipcs`.each do |line|
  next unless line =~ /^0x/
  shmid = (line.split)[1]
  puts "Removing shmid #{shmid}"
  system "ipcrm -m #{shmid}"
end
