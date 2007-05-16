#
# X-Chat Audacious for Audacious 1.4 and later
# This uses the native Audacious D-Bus interface.
#
# To consider later:
#   - support org.freedesktop.MediaPlayer (MPRIS)?
#
# This script is in the public domain.
#   $Id: xchat-audacious.py 4572 2007-05-16 07:24:43Z deitarion $
#

__module_name__ = "xchat-audacious"
__module_version__ = "1.0"
__module_description__ = "Get NP information from Audacious"

from dbus import Bus, DBusException, Interface
import xchat

# connect to DBus
bus = Bus(Bus.TYPE_SESSION)

def get_aud():
	try:
		return bus.get_object('org.atheme.audacious', '/org/atheme/audacious')
	except DBusException:
		print "\x02Either Audacious is not running or you have something wrong with your D-Bus setup."
		return None

def command_np(word, word_eol, userdata):
	aud = get_aud()
	if not aud:
		return xchat.EAT_ALL

	length = aud.SongLength(aud.Position())
	length = (length > 0) and ("%d:%02d" % (length / 60, length % 60)) or "stream"

	xchat.command("SAY [%s | %d:%02d/%s]" % (
		aud.SongTitle(aud.Position()).encode("utf8"),
		aud.Time() / 1000 / 60, aud.Time() / 1000 % 60,
		length))

	return xchat.EAT_ALL

def makeVoidCommand(cmd):
	def callback(word, word_eol, userdata):
		getattr(get_aud(), cmd, lambda: None)()
		return xchat.EAT_ALL
	return callback

command_next  = makeVoidCommand('Advance')
command_prev  = makeVoidCommand('Reverse')
command_pause = makeVoidCommand('Pause')
command_stop  = makeVoidCommand('Stop')
command_play  = makeVoidCommand('Play')

def command_send(word, word_eol, userdata):
	if len(word) < 2:
		print "You must provide a user to send the track to."
		return xchat.EAT_ALL

	aud = get_aud()
	if not aud:
		return xchat.EAT_ALL

	xchat.command('DCC SEND %s "%s"' % (word[1], aud.SongFilename(aud.Position()).encode("utf8")))
	return xchat.EAT_ALL

xchat.hook_command("NP", command_np, help="Displays current playing song.")
xchat.hook_command("NEXT", command_next, help="Advances in Audacious' playlist.")
xchat.hook_command("PREV", command_prev, help="Goes backwards in Audacious' playlist.")
xchat.hook_command("PAUSE", command_pause, help="Toggles paused status.")
xchat.hook_command("STOP", command_stop, help="Stops playback.")
xchat.hook_command("PLAY", command_play, help="Begins playback.")
xchat.hook_command("SENDTRACK", command_send, help="Sends the currently playing track to a user.")

print "xchat-audacious $Id: xchat-audacious.py 4572 2007-05-16 07:24:43Z deitarion $ loaded"
