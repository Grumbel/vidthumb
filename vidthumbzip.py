#!/usr/bin/env python3

# VidThumb - Video Thumbnailer
# Copyright (C) 2015 Ingo Ruhnke <grumbel@gmail.com>
#
# This program is free software: you can redistribute it and/or modify
# it under the terms of the GNU General Public License as published by
# the Free Software Foundation, either version 3 of the License, or
# (at your option) any later version.
#
# This program is distributed in the hope that it will be useful,
# but WITHOUT ANY WARRANTY; without even the implied warranty of
# MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
# GNU General Public License for more details.
#
# You should have received a copy of the GNU General Public License
# along with this program.  If not, see <http://www.gnu.org/licenses/>.


import os.path
import sys
import argparse
import tempfile
import subprocess
import zipfile


# FIXME: make generators configurable, take better care of the filenames
def do_thumbnails(filename, outputdir, zipfilename, generator):
    filename = os.path.abspath(filename)
    tmpdir = tempfile.mkdtemp()
    base = os.path.basename(filename)
    outputpattern = os.path.join(tmpdir, base + "-%08d.jpg")

    if generator == 'mpv':
        subprocess.check_call(['mpv',
                               '--ao', 'null',
                               '--quiet',
                               '--vo', 'image',
                               '--vo-image-outdir', tmpdir,
                               '--sstep', '60',
                               filename])
    elif generator == 'mplayer':
        subprocess.check_call(['mplayer',
                               '-nosound',
                               '-quiet',
                               '-vo', 'jpeg:outdir=' + tmpdir,
                               '-sstep', '60',
                               filename])
    elif generator == 'ffmpeg':
        subprocess.check_call(['ffmpeg',
                               '-i', filename,
                               '-vf', 'fps=1/60',
                               outputpattern])
    else:
        raise Exception("invalid generator: {}".format(generator))

    if zipfilename is None:
        zipfilename = os.path.join(outputdir, base + ".zip")

    print("Building .zip file: {}".format(zipfilename))
    with zipfile.ZipFile(zipfilename + ".part", mode='w') as archive:
        for f in os.listdir(tmpdir):
            ff = os.path.join(tmpdir, f)
            print("{}: adding {}".format(zipfilename, ff))
            archive.write(ff, arcname=f)
            os.remove(ff)
    os.rmdir(tmpdir)
    os.rename(zipfilename + ".part", zipfilename)


def main():
    parser = argparse.ArgumentParser(description='Create .zip files with video thumbnails')
    parser.add_argument('FILE', action='store', type=str, nargs='+',
                        help='Video file to thumbnail')
    parser.add_argument('-o', '--outputdir', metavar="DIR", type=str,
                        help="Write output .zip's to DIR")
    parser.add_argument('-O', '--outputfile', metavar="FILE", type=str,
                        help="Write output .zip's to FILE")
    parser.add_argument('--ffmpeg', dest='generator', action='store_const', const='ffmpeg',
                        help="Use ffmpeg for thumbnail generation")
    parser.add_argument('--mplayer', dest='generator', action='store_const', const='mplayer',
                        help="Use mplayer for thumbnail generation")
    parser.add_argument('--mpv', dest='generator', action='store_const', const='mpv',
                        help="Use mpv for thumbnail generation")
    args = parser.parse_args()

    if not args.generator:
        generator = 'mpv'
    else:
        generator = args.generator

    if args.outputdir is not None and args.outputfile is not None:
        raise Exception("can't mix --outputdir and --outputfile")

    if args.outputdir is None and args.outputfile is None:
        raise Exception("--outputdir or --outputfile required")

    if args.outputdir is not None and not os.path.isdir(args.outputdir):
        raise Exception("{}: not a directory".format(args.outputdir))

    if args.outputfile is not None and len(args.FILE) > 1:
        raise Exception("can't use --outputfile with multiple input files")

    for filename in args.FILE:
        do_thumbnails(filename, args.outputdir, args.outputfile, generator)


if __name__ == "__main__":
    main()


# EOF #
