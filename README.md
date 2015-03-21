VidThumb
========

A simple video thumbnailer:

    $ ./vidthumb --help
    Usage: ./vidthumb [OPTIONS] FILENAME

      -v, --verbose          Print verbose messages
      -d, --debug            Print debug messages
      -o, --output FILE      Write thumbnail to FILE
      -W, --width INT        Rescale the video to width
      -H, --height INT       Rescale the video to height
      -A, --ignore-aspect-ratio
                             Ignore aspect-ratio
      -p, --params PARAMS    Pass additional parameter to the thumbnailer (e.g. cols=5,rows=3)
      --fourd                Use fourd thumbnailer
                               parameter: slices=INT
      --grid                 Use grid thumbnailer (default)
                               parameter: cols=INT,rows=INT
      --directory            Use directory thumbnailer (default)
                               parameter: num=INT
      -t, --timeout SECONDS  Wait for SECONDS before giving up, -1 for infinity
      -T, --timestamp        Timestamp the frames
      -a, --accurate         Use accurate, but slow seeking
