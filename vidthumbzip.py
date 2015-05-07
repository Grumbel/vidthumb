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


def do_thumbnails(filename, outputdir):
    filename = os.path.abspath(filename)
    tmpdir = tempfile.mkdtemp()
    base = os.path.basename(filename)
    outputpattern = os.path.join(tmpdir, base + "-%03d.jpg")

    if True:
        subprocess.check_call(['mpv',
                               '--ao', 'null',
                               '--quiet',
                               '--vo', 'image:outdir=' + tmpdir,
                               '--sstep', '60',
                               filename])
    elif False:
        subprocess.check_call(['mplayer',
                               '-nosound',
                               '-quiet',
                               '-vo', 'jpeg:outdir=' + tmpdir,
                               '-sstep', '60',
                               filename])
    else:
        subprocess.check_call(['ffmpeg',
                               '-i', filename,
                               '-vf', 'fps=1/60',
                               outputpattern])

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
    parser.add_argument('-o', '--outputdir', metavar="DIR", type=str, required=True,
                        help="Write output .zip's to DIR")
    args = parser.parse_args()

    if not os.path.isdir(args.outputdir):
        raise Exception("{}: not a directory".format(args.outputdir))

    for filename in args.FILE:
        do_thumbnails(filename, args.outputdir)


if __name__ == "__main__":
    main()


# EOF #
