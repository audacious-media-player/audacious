#!/usr/bin/env python
"""g15_audacious.py
By: Stephan Sokolow (deitarion/SSokolow)

Audacious Media Player Monitor applet for the Logitech G15 Gaming Keyboard's LCD (via g15composer)

Requires Audacious 1.4 or newer. (Uses the D-Bus API)

TODO: 
- Every second update should skip the D-Bus calls. They are the biggest CPU drain.
  (I also only use a half-second update interval for the scroller. Feel free to 
   turn it off and use a 1-second interval.
- Wipe and redraw the entire screen at songchange. I'm not sure why yet, but some songs can cause mess-ups.
- Clean up this quick hack of a script.

Notes for people hacking this script:
- The LCD is 160px wide and 43px tall
- The Large  (2 or L) font is 7 pixels tall.
- The Medium (1 or M) font is 6 pixels tall.
- The Small  (0 or S) font is 5 pixels tall.
- When using the small font, the screen is 40 characters wide.
"""

control_pipe = "/var/run/g15composer"

ICON_DIMS = (7,7)
ICN_STOP = "1" * ICON_DIMS[0] * ICON_DIMS[1]
ICN_PAUSE = "0110110" * ICON_DIMS[1]
ICN_PLAY = (
"0100000" +
"0110000" +
"0111000" +
"0111100" +
"0111000" +
"0110000" +
"0100000" )

# 3x5 "Characters" (Icon glyphs sized to match the Small font)
CHAR_S_NOTE = "011010010110110"
CHAR_S_HALFTONE = "101010101010101"
CHAR_S_CROSS = "000010111010000"

# I prefer no icon when it's playing, so overwrite the play icon with a blank.
ICN_PLAY = '0' * ICON_DIMS[0] * ICON_DIMS[1]


import os, sys, time
from dbus import Bus, DBusException

message_pipe = os.path.join(os.environ["HOME"], ".g15_audacious_pipe")

def get_aud():
	try: return session_bus.get_object('org.atheme.audacious', '/org/atheme/audacious')
	except DBusException: return None

def draw_state(icon):
	msg_handle.write('PO 0 0 %d %d "%s"\n' % (ICON_DIMS[0], ICON_DIMS[1], icon))

session_bus = Bus(Bus.TYPE_SESSION)

if not os.path.exists(control_pipe) and not (os.path.isfile(control_pipe) and os.path.isdir(control_pipe)):
	print "ERROR: Not a g15composer control pipe: %s" % control_pipe
	sys.exit(1)

if os.path.exists(message_pipe) and not (os.path.isfile(control_pipe) and os.path.isdir(control_pipe)):
	os.remove(message_pipe)

try:
	file(control_pipe, 'w').write('SN "%s"\n' % message_pipe)
	time.sleep(0.5)
	msg_handle = file(message_pipe,'w')

	oldTitleString = ''
	while True:
		aud = get_aud()
		if aud:
			pos = aud.Position()

			lengthSecs = aud.SongLength(pos)
			length = (lengthSecs > 0) and ("%d:%02d" % (lengthSecs / 60, lengthSecs % 60)) or "stream"

			songTitle = aud.SongTitle(pos).encode("latin1", "replace")
			titleString = "%d. %s" % (pos, songTitle)

			playSecs = aud.Time() / 1000
			played = "%d:%02d" % (playSecs / 60, playSecs % 60)

			volume = aud.Volume()

			# Cache changes until I'm done making them
			msg_handle.write('MC 1\n')

			# Set up the static elements
			msg_handle.write('TO 0 0 2 1 "Now Playing"\n')
			msg_handle.write('TO 0 26 0 0 "Volume:"\n')

			# State Indicator
			if aud.Paused():
				draw_state(ICN_PAUSE)
			elif aud.Playing(): 
				draw_state(ICN_PLAY)
			else:
				draw_state(ICN_STOP)

			# Scroll the title string
			if len(titleString) > 40 and oldTitleString == titleString:
				tsTemp = titleString + '   '
				titleString = tsTemp[scrollPivot:] + tsTemp[:scrollPivot]
				scrollPivot = (scrollPivot < len(tsTemp)) and (scrollPivot + 1) or 0
			else:
				scrollPivot = 0
				oldTitleString = titleString

			# Title
			msg_handle.write('PB 0 9 160 14 0 1 1\n') # Wipe away any remnants of the old title that wouldn't be overwritten.
			msg_handle.write('TO 0 9 0 0 "%s"\n' % titleString)

			# Uncomment to place a note in the gap where the scroller wraps around
			#notePos = (len(titleString) - scrollPivot - 1) * 4
			#if len(titleString) > 40 and notePos <= 156:
			#	msg_handle.write('PO %d 9 3 5 "%s"\n' % (notePos, CHAR_S_NOTE))

			# Volume bar
			msg_handle.write('DB 29 28 159 29 1 %d 100 1\n' % ((volume[0] + volume[1])/2))

			# Progress Bar
			msg_handle.write('DB 0 33 159 41 1 %d %d 1\n' % (playSecs, lengthSecs))	# Progress Bar
			msg_handle.write('MX 1\n')
			msg_handle.write('TO 0 34 2 1 "Position: %s/%s"\n' % (played, length)) # Progress Text
			msg_handle.write('MX 0\n')

			# Push changes to the display and sleep for a second.
			msg_handle.write('MC 0\n')
			msg_handle.flush()
			time.sleep(0.5)
		else:
			msg_handle.write('PC 0\nTL "D-Bus Exception:" "Can\'t find Audacious"\n"')
finally:
	msg_handle.write("SC\n")
	msg_handle.close()
