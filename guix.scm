;; VidThumb - Video Thumbnailer
;; Copyright (C) 2020 Ingo Ruhnke <grumbel@gmail.com>
;;
;; This program is free software: you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation, either version 3 of the License, or
;; (at your option) any later version.
;;
;; This program is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.
;;
;; You should have received a copy of the GNU General Public License
;; along with this program.  If not, see <http://www.gnu.org/licenses/>.

;; Install with:
;; guix package --install-from-file=guix.scm

(set! %load-path
  (cons* "/ipfs/QmdHSWX34MXZeAgMuDuFwrYzSPf4fknX7E7cYJ7HDxaLrZ/guix-cocfree_0.0.0-62-g3b27118"
         %load-path))

(use-modules (guix build-system cmake)
             ((guix licenses) #:prefix license:)
             (guix packages)
             (gnu packages check)
             (gnu packages gcc)
             (gnu packages glib)
             (gnu packages gstreamer)
             (gnu packages gtk)
             (gnu packages pkg-config)
             (gnu packages pretty-print)
             (gnu packages python)
             (guix-cocfree utils))

(define %source-dir (dirname (current-filename)))

(define-public vidthumb
  (package
   (name "vidthumb")
   (version (version-from-source %source-dir))
   (source (source-from-source %source-dir #:version version))
   (arguments
    `(#:tests? #t
      #:configure-flags '("-DCMAKE_BUILD_TYPE=Release"
                          "-DBUILD_TESTS=ON")))
   (build-system cmake-build-system)
   (native-inputs
    `(("pkg-config" ,pkg-config)
      ("gcc" ,gcc-10) ; for <filesystem>
      ))
   (inputs
    `(("python" ,python)
      ("gstreamer" ,gstreamer)
      ("glib" ,glib)
      ("cairomm" ,cairomm)
      ("googletest" ,googletest)
      ("fmt" ,fmt)))
   (synopsis (synopsis-from-source %source-dir))
   (description (description-from-source %source-dir))
   (home-page (homepage-from-source %source-dir))
   (license license:gpl3+)))

vidthumb

;; EOF ;;
