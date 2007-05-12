#
# X-Chat Audacious for Audacious 1.4 and later
# This uses the native Audacious D-Bus interface.
#
# To consider later:
#   - support org.freedesktop.MediaPlayer (MPRIS)?
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

xchat.hook_command("NP", command_np, help="Displays current playing song.")
