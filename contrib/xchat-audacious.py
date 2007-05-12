#
# X-Chat Audacious for Audacious 1.4 and later
# This uses the native Audacious D-Bus interface.
#
# To consider later:
#   - support org.freedesktop.MediaPlayer (MPRIS)?
#
# This script is in the public domain.
#   $Id: xchat-audacious.py 4540 2007-05-12 04:35:06Z nenolod $
#

__module_name__ = "xchat-audacious"
__module_version__ = "1.0"
__module_description__ = "Get NP information from Audacious"

from dbus import Bus, Interface
import xchat

# connect to DBus
bus = Bus(Bus.TYPE_SESSION)

def command_np(word, word_eol, userdata):
	aud = bus.get_object('org.atheme.audacious', '/org/atheme/audacious')

	# this seems to be best, probably isn't!
	length = "stream"
	if aud.SongLength(aud.Position()) > 0:
		length = "%d:%02d" % (aud.SongLength(aud.Position()) / 60,
				      aud.SongLength(aud.Position()) % 60)

	xchat.command("SAY [%s | %d:%02d/%s]" % (
		aud.SongTitle(aud.Position()),
		aud.Time() / 1000 / 60, aud.Time() / 1000 % 60,
		length))

	return xchat.EAT_ALL

def command_next(word, word_eol, userdata):
	bus.get_object('org.atheme.audacious', '/org/atheme/audacious').Next()
	return xchat.EAT_ALL

def command_prev(word, word_eol, userdata):
	bus.get_object('org.atheme.audacious', '/org/atheme/audacious').Reverse()
	return xchat.EAT_ALL

def command_pause(word, word_eol, userdata):
	bus.get_object('org.atheme.audacious', '/org/atheme/audacious').Pause()
	return xchat.EAT_ALL

def command_stop(word, word_eol, userdata):
	bus.get_object('org.atheme.audacious', '/org/atheme/audacious').Stop()
	return xchat.EAT_ALL

def command_play(word, word_eol, userdata):
	bus.get_object('org.atheme.audacious', '/org/atheme/audacious').Play()
	return xchat.EAT_ALL

def command_send(word, word_eol, userdata):
	if len(word) < 1:
		print "You must provide a user to send the track to."
		return xchat.EAT_ALL

	aud = bus.get_object('org.atheme.audacious', '/org/atheme/audacious')
	xchat.command('DCC SEND %s "%s"' % (word[1], aud.SongFilename(aud.Position())))
	return xchat.EAT_ALL

xchat.hook_command("NP", command_np, help="Displays current playing song.")
xchat.hook_command("NEXT", command_next, help="Advances in Audacious' playlist.")
xchat.hook_command("PREV", command_prev, help="Goes backwards in Audacious' playlist.")
xchat.hook_command("PAUSE", command_pause, help="Toggles paused status.")
xchat.hook_command("STOP", command_stop, help="Stops playback.")
xchat.hook_command("PLAY", command_play, help="Begins playback.")
xchat.hook_command("SENDTRACK", command_send, help="Sends the currently playing track to a user.")

print "xchat-audacious $Id: xchat-audacious.py 4540 2007-05-12 04:35:06Z nenolod $ loaded"
