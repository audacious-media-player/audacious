;;;
;;; This code is derivative of guess.c of Gauche-0.8.7.
;;; The following is the original copyright notice.
;;;

;;;
;;; Auxiliary script to generate japanese code guessing table
;;;
;;;   Copyright (c) 2000-2003 Shiro Kawai, All rights reserved.
;;;
;;;   Redistribution and use in source and binary forms, with or without
;;;   modification, are permitted provided that the following conditions
;;;   are met:
;;;
;;;   1. Redistributions of source code must retain the above copyright
;;;      notice, this list of conditions and the following disclaimer.
;;;
;;;   2. Redistributions in binary form must reproduce the above copyright
;;;      notice, this list of conditions and the following disclaimer in the
;;;      documentation and/or other materials provided with the distribution.
;;;
;;;   3. Neither the name of the authors nor the names of its contributors
;;;      may be used to endorse or promote products derived from this
;;;      software without specific prior written permission.
;;;
;;;   THIS SOFTWARE IS PROVIDED BY THE COPYRIGHT HOLDERS AND CONTRIBUTORS
;;;   "AS IS" AND ANY EXPRESS OR IMPLIED WARRANTIES, INCLUDING, BUT NOT
;;;   LIMITED TO, THE IMPLIED WARRANTIES OF MERCHANTABILITY AND FITNESS FOR
;;;   A PARTICULAR PURPOSE ARE DISCLAIMED. IN NO EVENT SHALL THE COPYRIGHT
;;;   OWNER OR CONTRIBUTORS BE LIABLE FOR ANY DIRECT, INDIRECT, INCIDENTAL,
;;;   SPECIAL, EXEMPLARY, OR CONSEQUENTIAL DAMAGES (INCLUDING, BUT NOT LIMITED
;;;   TO, PROCUREMENT OF SUBSTITUTE GOODS OR SERVICES; LOSS OF USE, DATA, OR
;;;   PROFITS; OR BUSINESS INTERRUPTION) HOWEVER CAUSED AND ON ANY THEORY OF
;;;   LIABILITY, WHETHER IN CONTRACT, STRICT LIABILITY, OR TORT (INCLUDING
;;;   NEGLIGENCE OR OTHERWISE) ARISING IN ANY WAY OUT OF THE USE OF THIS
;;;   SOFTWARE, EVEN IF ADVISED OF THE POSSIBILITY OF SUCH DAMAGE.
;;;
;;;  $Id: guess.scm,v 1.3 2003/07/05 03:29:10 shirok Exp $
;;;

;;;
;;; To rebuild DFA tables, install Gauche and run the following in
;;; the libguess source directory:
;;;
;;; $ gosh guess.scm guess_tab.c
;;;
;;; DFAs are defined such as:
;;;
;;; (define-dfa c-prefix-name
;;;  (init
;;;   (((byte sequence)) step-name  confidence-score)
;;;   (((byte sequence)) step-name  confidence-score)))
;;;
;;; DFAs can have infinite steps, the table defines how the DFA behaves.
;;; 'init' is the name of the first step.
;;;

(use srfi-1)
(use gauche.sequence)

;; This is a simple state machine compiler.
;;
;; <state-machine> : (define-dfa <name> <state> ...)
;; <state> : (<name> (<input-set> <next-state> <score>) ...)
;; <name>  : symbol
;; <next-state> : symbol
;; <score> : real
;; <input-set> : (<byte-or-range> ...)
;; <byte-or-range> : <byte> | (<byte> <byte>)
;; <byte> : integer between 0 and #xff | ASCII char
;;
;; When evaluated, the DFA generates a state transition table in
;; C source format.

(define-class <dfa> ()
  ((name    :init-keyword :name   :accessor name-of)
   (states  :init-keyword :states :accessor states-of)
   (instances :allocation :class  :init-value '())))

(define-class <state> ()
  ((name    :init-keyword :name   :accessor name-of)
   (index   :init-keyword :index  :accessor index-of)
   (arcs    :init-keyword :arcs   :accessor arcs-of :init-value '())))

(define-class <arc> ()
  ((from-state :init-keyword :from-state :accessor from-state-of)
   (to-state   :init-keyword :to-state   :accessor to-state-of)
   (ranges     :init-keyword :ranges     :accessor ranges-of)
   (index      :init-keyword :index      :accessor index-of)
   (score      :init-keyword :score      :accessor score-of)))

;; Create DFA

(define-syntax define-dfa
  (syntax-rules ()
    ((_ name . states)
     (define name (make <dfa>
                    :name 'name
                    :states (resolve-states 'states))))))

(define-method initialize ((self <dfa>) initargs)
  (next-method)
  (slot-push! self 'instances self))

(define (all-dfas) (reverse (class-slot-ref <dfa> 'instances)))

(define (resolve-states state-defs)
  (let ((states (map (lambda (d i) (make <state> :name (car d) :index i))
                     state-defs
                     (iota (length state-defs)))))
    (fold (lambda (s d i)
            (let1 num-arcs (length (cdr d))
              (set! (arcs-of s)
                    (map (lambda (arc aindex)
                           (make <arc>
                             :from-state s
                             :to-state (or (find (lambda (e)
                                                   (eq? (name-of e) (cadr arc)))
                                                 states)
                                           (error "no such state" (cadr arc)))
                             :ranges (car arc)
                             :index aindex
                             :score (caddr arc)))
                         (cdr d)
                         (iota num-arcs i)))
              (+ i num-arcs)))
          0
          states state-defs)
    states))

;; Emit state table
(define (emit-dfa-table dfa)
  (format #t "static signed char guess_~a_st[][256] = {\n" (name-of dfa))
  (for-each emit-state-table (states-of dfa))
  (print "};\n")
  (format #t "static guess_arc guess_~a_ar[] = {\n" (name-of dfa))
  (for-each emit-arc-table
            (append-map arcs-of (states-of dfa)))
  (print "};\n")
  )

(define (emit-state-table state)
  (define (b2i byte)                    ;byte->integer
    (if (char? byte) (char->integer byte) byte))
  (let1 arc-vec (make-vector 256 -1)
    (dolist (br (arcs-of state))
      (dolist (range (ranges-of br))
        (if (pair? range)
            (vector-fill! arc-vec (index-of br)
                          (b2i (car range)) (+ (b2i (cadr range)) 1))
            (set! (ref arc-vec (b2i range)) (index-of br)))))
    (format #t " { /* state ~a */" (name-of state))
    (dotimes (i 256)
      (when (zero? (modulo i 16)) (newline))
      (format #t " ~2d," (ref arc-vec i)))
    (print "\n },")
    ))

(define (emit-arc-table arc)
  (format #t " { ~2d, ~5s }, /* ~a -> ~a */\n"
          (index-of (to-state-of arc))
          (score-of arc)
          (name-of (from-state-of arc))
          (name-of (to-state-of arc))))
;;
;; main
;;

(define (main args)
  (unless (= (length args) 2)
    (error (format #f "usage: ~a <output-file.c>" (car args))))
  (with-output-to-file (cadr args)
    (lambda ()
      (print "/* State transition table for character code guessing */")
      (print "/* This file is automatically generated by guess.scm */")
      (print "")
      (print "#include \"dfa.h\"")
      (newline)
      (for-each emit-dfa-table (all-dfas))))
  0)

;;;============================================================
;;; DFA definitions
;;;

;;;
;;; EUC-JP
;;;

(define-dfa eucj
  ;; first byte
  (init
   (((#x00 #x7f)) init         1.0)   ; ASCII range
   ((#x8e)        jis0201_kana 0.8)   ; JISX 0201 kana
   ((#x8f)        jis0213_2    0.95)  ; JISX 0213 plane 2
   (((#xa1 #xfe)) jis0213_1    1.0))   ; JISX 0213 plane 1
  ;; jis x 0201 kana
  (jis0201_kana
   (((#xa1 #xdf)) init         1.0))
  ;; jis x 0208 and jis x 0213 plane 1
  (jis0213_1
   (((#xa1 #xfe)) init         1.0))
  ;; jis x 0213 plane 2
  (jis0213_2
   (((#xa1 #xfe)) init         1.0)))

;;;
;;; Shift_JIS
;;;

(define-dfa sjis
  ;; first byte
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   (((#x81 #x9f) (#xe1 #xef)) jis0213 1.0) ;jisx0213 plane 1
   (((#xa1 #xdf)) init         0.8)     ;jisx0201 kana
   (((#xf0 #xfc)) jis0213      0.95)    ;jisx0213 plane 2
   (((#xfd #xff)) init         0.8))    ;vendor extension
  (jis0213
   (((#x40 #x7e) (#x80 #xfc)) init 1.0)))

;;;
;;; UTF-8
;;;

(define-dfa utf8
  (init
   (((#x00 #x7f)) init         1.0)
   (((#xc2 #xdf)) 1byte_more   1.0)
   (((#xe0 #xe0)) 2byte_more1  1.0)
   (((#xe1 #xec)) 2byte_more2  1.0)
   (((#xed #xed)) 2byte_more3  1.0)
   (((#xee #xef)) 2byte_more2  1.0)
   (((#xf0 #xf0)) 3byte_more1  1.0)
   (((#xf1 #xf3)) 3byte_more2  1.0)
   (((#xf4 #xf4)) 3byte_more3  1.0))
  (1byte_more
   (((#x80 #xbf)) init         1.0))
  (2byte_more1
   (((#xa0 #xbf)) 1byte_more   1.0))
  (2byte_more2
   (((#x80 #xbf)) 1byte_more   1.0))
  (2byte_more3
   (((#x80 #x9f)) 1byte_more   1.0))
  (3byte_more1
   (((#x90 #xbf)) 2byte_more2  1.0))
  (3byte_more2
   (((#x80 #xbf)) 2byte_more2  1.0))
  (3byte_more3
   (((#x80 #x8f)) 2byte_more2  1.0)))

;;;
;;; UCS-2LE
;;;
(define-dfa ucs2le
  (init
   ((#xfe) bom-be 1.0)
   ((#xff) bom-le 1.0)
   (((#x00 #xfd)) byte2 1.0))
  (bom-le
   (((#x00 #xff)) init 1.0))
  (bom-be
   (((#x00 #xfe)) init 1.0))  ;; if be (0xfeff), die.
  (byte2
   (((#x00 #xff)) init 1.0)))

;;;
;;; UCS-2BE
;;;
(define-dfa ucs2be
  (init
   ((#xfe) bom-be 1.0)
   ((#xff) bom-le 1.0)
   (((#x00 #xfd)) byte2 1.0))
  (bom-le
   (((#x00 #xfd)) init 1.0)
   ((#xff) init 1.0)) ;; if le (0xfffe), die.
  (bom-be
   (((#x00 #xff)) init 1.0))
  (byte2
   (((#x00 #xff)) init 1.0)))

;;;
;;; UTF-16
;;;
(define-dfa utf16
  (init
   ((#xfe) bom-be 1.0)
   ((#xff) bom-le 1.0))
  (init1
   (((#x00 #xff)) byte2 1.0))
  (bom-be
   ((#xff) init1 1.0))
  (bom-le
   ((#xfe) init1 1.0))
  (byte2
   (((#x00 #xff)) init1 1.0)))

;;;
;;; ISO2022JP (JIS)
;;;

;; NB: for now, we just check the sequence of <ESC> $ or <ESC> '('.
'(define-dfa jis
  (init
   ((#x1b)        esc          1.0)
   (((#x00 #x1a)  (#x1c #x1f)) init 1.0) ;C0
   (((#x20 #x7f)) init         1.0)      ;ASCII
   (((#xa1 #xdf)) init         0.7))     ;JIS8bit kana
  (esc
   ((#x0d #x0a)   init         0.9)      ;cancel
   ((#\( )        esc-paren    1.0)
   ((#\$ )        esc-$        1.0)
   ((#\& )        esc-&        1.0))
  (esc-paren
   ((#\B #\J #\H) init         1.0)
   ((#\I)         jis0201kana  0.8))
  (esc-$
   ((#\@ #\B)     kanji        1.0)
   ((#\( )        esc-$-paren  1.0))
  (esc-$-paren
   ((#\D #\O #\P) kanji        1.0))
  (esc-&
   ((#\@ )        init         1.0))
  (jis0201kana
   ((#x1b)        esc          1.0)
   (((#x20 #x5f)) jis0201kana  1.0))
  (kanji
   ((#x1b)        esc          1.0)
   (((#x21 #x7e)) kanji-2      1.0))
  (kanji-2
   (((#x21 #x7e)) kanji        1.0)) )

;;;
;;; Big5
;;;

(define-dfa big5
  ;; first byte
  (init
   (((#x00 #x7f)) init         1.0)      ;ascii
   (((#xa1 #xfe)) 2byte        1.0))     ;big5-2byte
  (2byte
   (((#x40 #x7e) (#xa1 #xfe)) init 1.0)))

;;;
;;; GB2312 (EUC-CN?)
;;;

(define-dfa gb2312
  ;; first byte
  (init
   (((#x00 #x7f)) init         1.0)      ;ascii
   (((#xa1 #xfe)) 2byte        1.0))     ;gb2312 2byte
  (2byte
   (((#xa1 #xfe)) init 1.0)))

;;;
;;; GB18030
;;;

(define-dfa gb18030
  ;; first byte
  (init
   (((#x00 #x80)) init         1.0)     ;ascii
   (((#x81 #xfe)) 2byte        1.0)     ;gb18030 2byte
   (((#x81 #xfe)) 4byte2       1.0))     ;gb18030 2byte
  (2byte
   (((#x40 #x7e) (#x80 #xfe)) init 1.0))
  (4byte2
   (((#x30 #x39)) 4byte3 1.0))
  (4byte3
   (((#x81 #xfe)) 4byte4 1.0))
  (4byte4
   (((#x30 #x39)) init   1.0)) )

;;;
;;; EUC-KR
;;;

(define-dfa euck
  ;; first byte
  (init
   (((#x00 #x7f)) init      1.0)   ; ASCII range
   (((#xa1 #xfe)) ks1001    1.0))   ; KSX 1001
  ;; ks x 1001
  (ks1001
   (((#xa1 #xfe)) init      1.0)))

;;;
;;; Johab
;;;

(define-dfa johab
  ;; first byte
  (init
   (((#x00 #x7f)) init         1.0)   ; ASCII range
   (((#x84 #xd3)) jamo51       1.0)   ; jamo51
   (((#xd8 #xde) (#xe0 #xf9)) jamo42  0.95))   ; jamo42
  ;; second byte
  (jamo51
   (((#x41 #x7e) (#x81 #xfe)) init         1.0))
  (jamo42
   (((#x31 #x7e) (#x91 #xfe)) init         1.0)))






;;;
;;; arabic
;;;

(define-dfa iso8859_6
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   ((#xa0)        init         1.0)
   ((#xa4)        init         1.0)
   ((#xac)        init         1.0)
   ((#xad)        init         1.0)     ;SHY xxx
   ((#xbb)        init         1.0)
   ((#xbf)        init         1.0)
   (((#xc1 #xda)) init         1.0)
   (((#xe0 #xf2)) init         1.0)))

(define-dfa cp1256
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   (((#x80 #xff)) init         1.0)))   ;high bit


;;;
;;; greek
;;;

(define-dfa iso8859_7
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   (((#xa0 #xad)) init         1.0)
   (((#xaf #xd1)) init         1.0)
   (((#xd3 #xfe)) init         1.0)))

(define-dfa cp1253
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   ((#x80)        init         1.0)
   (((#x82 #x87)) init         1.0)
   ((#x89)        init         1.0)
   ((#x8b)        init         1.0)
   (((#x91 #x97)) init         1.0)
   ((#x99)        init         1.0)
   ((#x9b)        init         1.0)
   (((#xa0 #xa9)) init         1.0)
   (((#xab #xd1)) init         1.0)
   (((#xd3 #xfe)) init         1.0)))

;;;
;;; hebrew
;;;

(define-dfa iso8859_8
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   ((#xa0)        init         1.0)
   (((#xa2 #xbe)) init         1.0)
   (((#xdf #xfa)) init         1.0)
   (((#xfd #xfe)) init         1.0)))

(define-dfa cp1255
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   ((#x80)        init         1.0)
   (((#x82 #x89)) init         1.0)
   ((#x8b)        init         1.0)
   (((#x91 #x99)) init         1.0)
   ((#x9b)        init         1.0)
   (((#xa0 #xc9)) init         1.0)
   (((#xcb #xd8)) init         1.0)
   (((#xe0 #xfa)) init         1.0)
   (((#xfd #xfe)) init         1.0)))

;;;
;;; turkish
;;;

(define-dfa iso8859_9
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   (((#xa0 #xff)) init         1.0)))

(define-dfa cp1254
  (init
   (((#x00 #x7f)) init         1.0)     ;ascii
   ((#x80)        init         1.0)
   (((#x82 #x8c)) init         1.0)
   (((#x91 #x9c)) init         1.0)
   (((#x9f #xff)) init         1.0)))

;;;
;;; russian
;;;

(define-dfa iso8859_5
  (init
   (((#x00 #x7f)) init         1.0)     ; ascii
   (((#xa0 #xff)) init         1.0)))
  
(define-dfa koi8_r
  (init
   (((#x20 #x7f)) init         1.0)     ; ascii
   ((#xa3)        init         1.0)
   ((#xb3)        init         1.0)
   (((#xc0 #xff)) init         1.0)))

(define-dfa koi8_u
  (init
   (((#x20 #x7f)) init         1.0)     ; ascii
   (((#xa3 #xa4)) init         1.0)
   (((#xa6 #xa7)) init         1.0)
   ((#xad)        init         1.0)
   (((#xb3 #xb4)) init         1.0)
   (((#xb6 #xb7)) init         1.0)
   ((#xbd)        init         1.0)
   (((#xc0 #xff)) init         1.0)))

(define-dfa cp866
  (init
   (((#x00 #x7f)) init         1.0)     ; ascii
   (((#x80 #xaf)) init         1.0)
   (((#xe0 #xf7)) init         1.0)))

(define-dfa cp1251
  (init
   (((#x00 #x7f)) init         1.0)     ; ascii
   (((#x80 #x81)) init         1.0)
   ((#x83)        init         1.0)
   ((#x8a)        init         1.0)
   (((#x8c #x90)) init         1.0)
   ((#x9a)        init         1.0)
   (((#x9c #xa3)) init         1.0)
   ((#xa5)        init         1.0)
   ((#xa8)        init         1.0)
   ((#xaa)        init         1.0)
   ((#xaf)        init         1.0)
   (((#xb2 #xb4)) init         1.0)
   ((#xb8)        init         1.0)
   ((#xba)        init         1.0)
   (((#xbc #xff)) init         1.0)))

;;;
;;; polish
;;;

(define-dfa iso8859_2
 (init
  (((#x20 #x7e)) init          1.0)
  ((#xa1)        init          1.0)
  ((#xa3)        init          1.0)
  (((#xa5 #xa6)) init          1.0)
  (((#xa9 #xac)) init          1.0)
  (((#xae #xaf)) init          1.0)
  ((#xb1)        init          1.0)
  ((#xb3)        init          1.0)
  (((#xb5 #xb6)) init          1.0)
  (((#xb9 #xbc)) init          1.0)
  (((#xbe #xbf)) init          1.0)
  (((#xc0 #xd6)) init          1.0)
  (((#xd8 #xf6)) init          1.0)
  (((#xf8 #xfe)) init          1.0)))

(define-dfa cp1250
 (init
  (((#x20 #x7e)) init          1.0)
  (((#x8a #x8f)) init          1.0)
  ((#x9a)        init          1.0)
  (((#x9c #xa1)) init          1.0)
  ((#xa3)        init          1.0)
  ((#xa5)        init          1.0)
  ((#xaa)        init          1.0)
  ((#xaf)        init          1.0)
  ((#xb3)        init          1.0)
  (((#xb9 #xba)) init          1.0)
  ((#xbc)        init          1.0)
  (((#xbe #xbf)) init          1.0)
  (((#xc0 #xd6)) init          1.0)
  (((#xd8 #xf6)) init          1.0)
  (((#xf8 #xfe)) init          1.0)))

;;;
;;; baltic (estonia/latvia/lithuania)
;;;

(define-dfa iso8859_13
 (init
  (((#x20 #x7e)) init          1.0)
  ((#xa8)        init          1.0)
  ((#xaa)        init          1.0)
  ((#xaf)        init          1.0)
  ((#xb8)        init          1.0)
  ((#xba)        init          1.0)
  ((#xbf)        init          1.0)
  (((#xc0 #xd6)) init          1.0)
  (((#xd8 #xf6)) init          1.0)
  (((#xf8 #xfe)) init          1.0)))

(define-dfa cp1257
 (init
  (((#x20 #x7e)) init          1.0)
  ((#x80)        init          1.0)
  ((#x82)        init          1.0)
  (((#x84 #x87)) init          1.0)
  ((#x89)        init          1.0)
  ((#x8b)        init          1.0)
  (((#x8d #x8f)) init          1.0)
  (((#x91 #x97)) init          1.0)
  ((#x99)        init          1.0)
  ((#x9b)        init          1.0)
  (((#x9d #x9e)) init          1.0)
  ((#xa0)        init          1.0)
  (((#xa2 #xa4)) init          1.0)
  (((#xa6 #xff)) init          1.0)))

