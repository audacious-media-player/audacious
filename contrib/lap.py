#!/usr/bin/env python
"""Locate and Play
Author: Stephan Sokolow (deitarion/SSokolow)
License: GNU GPL-2
Version: 1.0

Description:
- A quick wrapper to make playing songs via the local command quick and easy.
- Works with any player which implements the MPRIS standard for remote control via D-Bus.
- Accepts multiple space- and/or comma-separated choices after presenting the results.
- Can enqueue or enqueue and play.
- Can show full paths or just filenames.
- Will behave in a sane fashion when asked to enqueue and play multiple files.

TODO: 
 - Complete the list of extensions for ModPlug, Embedded MPlayer (plugins-ugly), and UADE (3rd-party)
 - Support sticking a 'q' in the result to choose enqueue on the fly
 - Support an "all" keyword and an alternative to Ctrl+C for cancelling. (maybe 0)
"""

ADLIB_EXTS    = ['.a2m', '.adl',  '.adlib', '.amd', '.bam', '.cff', '.cmf', '.d00', '.dfm',  '.dmo', '.dro', '.dtm', '.hsc', '.hsp',
                 '.jbm', '.ksm',  '.laa',   '.lds', '.m',   '.mad', '.mkj', '.msc', '.mtk',  '.rad', '.raw', '.rix', '.rol', '.sat', 
                 '.sa2', '.sci',  '.sng',   '.imf', '.wlf', '.xad', '.xsm']
CONSOLE_EXTS  = ['.adx', '.ay',   '.gbs',   '.gym', '.hes', '.kss', '.minipsf',     '.nsf', '.nsfe', '.psf', '.sap', '.sid', '.spc', 
                 '.vgm', '.vgz',  '.vtx',   '.ym']
MIDI_EXTS     = ['.mid', '.midi', '.rmi']
MODULE_EXTS   = [ '.it', ',mod',  '.s3m',   '.stm', '.xm']
PLAYLIST_EXTS = ['.cue', '.m3u',  '.pls',   '.xspf']
WAVEFORM_EXTS = ['.aac', '.aif',  '.aiff',  '.au', '.flac', '.m4a', '.mp2', '.mp3', '.mpc', '.ogg', '.snd', '.tta', '.voc', '.wav', 
                 '.wma', '.wv']

# Edit these lines to choose the kind of files to be filtered for. By default, playlist extensions are excluded.
OK_EXTS = WAVEFORM_EXTS + MODULE_EXTS + CONSOLE_EXTS + MIDI_EXTS + ADLIB_EXTS
# If you want true format filtering, YOU write the mimetype cache.
USE_PAGER = False  # Should we page output if it's more than a screenful?

locate_command = ['locate', '-i']

# ========== Configuration Ends ==========

import fnmatch, optparse, os, subprocess
from dbus import Bus, DBusException

# Support audtool as a fallback but don't depend on it
try: import subprocess
except ImportError: pass

# Use readline if available but don't depend on it
try: import readline
except ImportError: pass

# connect to DBus
bus = Bus(Bus.TYPE_SESSION)

def get_results(query):
	"""Given a query or series of queries for the locate command, run them."""
	results, cmd = [], locate_command + (isinstance(query, basestring) and [query] or query)
	for line in subprocess.Popen(cmd, stdout=subprocess.PIPE).stdout:
		result = line.strip()
		if os.path.splitext(result)[1] in OK_EXTS:
			results.append(result)
	results.sort()
	return results

def filter(results, filters):
	for filter in filters:
		results = [x for x in results if fnmatch.fnmatch(x.lower(), '*%s*' % filter.lower())]
	return results

def makeMenu(results, strip_path=True):
	for pos, val in enumerate(results):
		val = strip_path and os.path.split(val)[1] or val
		print "%3d) %s" % (pos+1, val)

def addTrack(path, play=False):
	try:
		file_url = 'file://' + path
		mp = bus.get_object('org.freedesktop.MediaPlayer', '/TrackList')
		mp.AddTrack(file_url, play)
	except DBusException:
		try:
			if subprocess.call(['audtool','playlist-addurl',file_url]):
				print "ERROR: audtool fallback returned an error for: %s" % file_url
			elif play:
				os.system('audtool playlist-jump `audtool playlist-length`; audtool playback-play')
		except OSError:
			print "ERROR: Unable to call audtool as a fallback for: %s" % file_url

def parseChoice(inString):
	try:
		return [int(inString)]
	except ValueError:
		choices = []
		for x in inString.replace(',',' ').split():
			try: choices.append(int(x))
			except ValueError: print "Not an integer: %s" % x
		return choices

if __name__ == '__main__':
    try:
	op = optparse.OptionParser()
	op.add_option("-q", "--enqueue", action="store_true", dest="enqueue", default=False,
	               help="Don't start the song playing after enqueueing it.")
	op.add_option("-p", "--show_path", action="store_true", dest="show_path", default=False,
	               help="Show the full path to each result.")

	(opts, args) = op.parse_args()
	
	results = (len(args) > 0) and get_results(args.pop(0)) or []
	results = filter(results, args)

	def takeChoice(index, play=False):
		index = index - 1
		if index >= 0 and index < len(results):
			addTrack(results[index], play)
		else:
			print "Invalid result index: %s" % (index + 1)

	if len(results):
		makeMenu(results, not opts.show_path)
		choices = parseChoice(raw_input("Choice(s) (Ctrl+C to cancel): "))

		if len(choices):
			takeChoice(choices.pop(0), not opts.enqueue)
		for item in choices:
			takeChoice(item, False) # This ensures proper behaviour with no --enqueue and multiple choices.
	else:
		print "No Results"
    except KeyboardInterrupt:
	pass
