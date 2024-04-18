# ----------------------------
# Program Options
# ----------------------------

NAME = CSOLVER
ICON = icon.png
DESCRIPTION  = "Solve Equations"
COMPRESSED = NO

CFLAGS = -Wall -Wextra -Oz
CXXFLAGS = -Wall -Wextra -Oz

# ----------------------------

include $(shell cedev-config --makefile)
