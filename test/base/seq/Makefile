CFLAGS = -Wall -Wextra -Wundef

targets = complainer generator queue-timer queue-timer-jack

all : $(targets)

% : %.c
	cc $(CFLAGS) $< -o $@ -ljack -lasound

.PHONY : clean
clean :
	rm $(targets)

