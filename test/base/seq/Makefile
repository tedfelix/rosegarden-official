
all:	complainer generator queue-timer queue-timer-jack

%:	%.c
	cc $< -o $@ -ljack -lasound

.PHONY : clean
clean :
	rm complainer generator queue-timer queue-timer-jack

