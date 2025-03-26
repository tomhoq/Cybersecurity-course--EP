LINK_TARGET = \
			client \
			server \
			fancyclient

# Here is a Make Macro that uses the backslash to extend to multiple lines.
# This allows quick modification of more object files.

OBJS = \
       client.o \
        server.o \
		fancyclient.o

# Here is a Make Macro defined by two Macro Expansions.
# A Macro Expansion may be treated as a textual replacement of the Make Macro.
# Macro Expansions are introduced with $ and enclosed in (parentheses).

REBUILDABLES = $(OBJS) $(LINK_TARGET)


all : $(LINK_TARGET)

# Make Macros do not need to be defined before their Macro Expansions,
# but they normally should be defined before they appear in any Rules.
# Consequently Make Macros often appear first in a Makefile.

# Here is a simple Rule (used for "cleaning" your build environment).
# It has a Target named "clean" (left of the colon ":" on the first line),
# no Dependencies (right of the colon),
# and two Commands (indented by tabs on the lines that follow).
# The space before the colon is not required but added here for clarity.

clean:
	rm -f $(REBUILDABLES)

# There is no required order to the list of rules as they appear in the Makefile.
# Make will build its own dependency tree and only execute each rule only once
# its dependencies' rules have been executed successfully.

# Here is a Rule that uses some built-in Make Macros in its command:
# $@ expands to the rule's target, in this case "pointer".
# $^ expands to the rule's dependencies, in this case the two files
# pointer.o and idserver.o
# pointer.o and idserver.o
client: client.o
	cc -g -o $@ $^

server: server.o
	cc -g -o $@ $^

fancyclient: fancyclient.o
	cc -g -o $@ $^
# Here is a Pattern Rule, often used for compile-line.
# It says how to create a file with a .o suffix, given a file with a .c suffix.
# The rule's command uses some built-in Make Macros:
# $@ for the pattern-matched target
# $lt; for the pattern
# Here is a Pattern Rule, often used for compile-line.
# It says how to create a file with a .o suffix, given a file with a .c suffix.
# The rule's command uses some built-in Make Macros:
# $@ for the pattern-matched target
# $lt; for the pattern-matched dependency

%.o : %.c
	cc -g  -Wall -o $@ -c $<

# These are Dependency Rules, which are rules without any command.
# Dependency Rules indicate that if any file to the right of the colon changes,
# the target to the left of the colon should be considered out-of-date.
# The commands for making an out-of-date target up-to-date may be found elsewhere
# (in this case, by the Pattern Rule above).
# Dependency Rules are often used to capture header file dependencies.

client.o: client.c 
fancyclient.o: fancyclient.c
server.o: server.c
