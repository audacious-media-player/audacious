#
# X-Chat Audacious for Audacious 1.4 and later
# This uses the native Audacious D-Bus interface.
#
# To consider later:
#   - support org.freedesktop.MediaPlayer (MPRIS)?
#
# This script is in the public domain.
#   $Id: xchat-audacious.py 4574 2007-05-16 07:46:17Z deitarion $
#

__module_name__ = "xchat-audacious"
__module_version__ = "1.0.1"
__module_description__ = "Get NP information from Audacious"

from dbus import Bus, DBusException
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
	if aud:
		pos = aud.Position()

		length = aud.SongLength(pos)
		length = (length > 0) and ("%d:%02d" % (length / 60, length % 60)) or "stream"

		playSecs = aud.Time() / 1000
		xchat.command("SAY [%s | %d:%02d/%s]" % (
			aud.SongTitle(pos).encode("utf8"),
			playSecs / 60, playSecs % 60, length))
	return xchat.EAT_ALL

def makeVoidCommand(cmd):
	def callback(word, word_eol, userdata):
		getattr(get_aud(), cmd, lambda: None)()
		return xchat.EAT_ALL
	return callback

def command_send(word, word_eol, userdata):
	if len(word) < 2:
		print "You must provide a user to send the track to."
		return xchat.EAT_ALL

	aud = get_aud()
	if aud:
		xchat.command('DCC SEND %s "%s"' % (word[1], aud.SongFilename(aud.Position()).encode("utf8")))
	return xchat.EAT_ALL

xchat.hook_command("NP",    command_np,                 help="Displays current playing song.")
xchat.hook_command("NEXT",  makeVoidCommand('Advance'), help="Advances in Audacious' playlist.")
xchat.hook_command("PREV",  makeVoidCommand('Reverse'), help="Goes backwards in Audacious' playlist.")
xchat.hook_command("PAUSE", makeVoidCommand('Pause'),   help="Toggles paused status.")
xchat.hook_command("STOP",  makeVoidCommand('Stop'),    help="Stops playback.")
xchat.hook_command("PLAY",  makeVoidCommand('Play'),    help="Begins playback.")
xchat.hook_command("SENDTRACK", command_send, help="Syntax: /SENDTRACK <nick>\nSends the currently playing track to a user.")

# IRC+PP support section

# XChat is lame and does not give us a server list.
def get_servers():
	chanlist = xchat.get_list("channels")
	servlist = []

	for i in chanlist:
		if i.server not in servlist:
			servlist.append(i.server)

	return servlist

ignore_services = 0;
def ignore_service_errors_cb(word, word_eol, userdata):
	global ignore_services

	if ignore_services == 1:
		return xchat.EAT_ALL

	return xchat.EAT_NONE

#xchat.hook_print("Notice", ignore_service_errors_cb)

def unset_ignore_services(userdata=None):
	global ignore_services

	ignore_services = 0
	return 1

last_title = None

def presence_notification_dispatch(userdata=None):
	global ignore_services, last_title

	aud = get_aud()

	ignore_services = 1
	if aud:
		pos = aud.Position()

		title = aud.SongTitle(pos).encode("utf8")

		if title != last_title:
			slist = get_servers()
			for i in slist:
				ctx = xchat.find_context(i)

				ctx.command("nickserv set qproperty np %s" % (title))

			last_title = title

	return 1

#presence_notification_dispatch()
#xchat.hook_timer(3000, presence_notification_dispatch)
#xchat.hook_timer(500, unset_ignore_services)

print "xchat-audacious $Id: xchat-audacious.py 4574 2007-05-16 07:46:17Z deitarion $ loaded"
