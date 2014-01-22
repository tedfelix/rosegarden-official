;;;_ auto-insert-rosegarden-templates.el --- Auto-inserts for Rosegarden

;;;_. Headers
;;;_ , License
;; Copyright (C) 2011  Tom Breton (Tehom)

;; Author: Tom Breton (Tehom) <tehom@panix.com>
;; Keywords: local

;; This file is free software; you can redistribute it and/or modify
;; it under the terms of the GNU General Public License as published by
;; the Free Software Foundation; either version 2, or (at your option)
;; any later version.

;; This file is distributed in the hope that it will be useful,
;; but WITHOUT ANY WARRANTY; without even the implied warranty of
;; MERCHANTABILITY or FITNESS FOR A PARTICULAR PURPOSE.  See the
;; GNU General Public License for more details.

;; You should have received a copy of the GNU General Public License
;; along with GNU Emacs; see the file COPYING.  If not, write to
;; the Free Software Foundation, Inc., 59 Temple Place - Suite 330,
;; Boston, MA 02111-1307, USA.

;;;_ , Commentary:

;; This file contains no entry points.  It provides auto-insertable
;; templates for `auto-insert-choose'.
;;
;; If you haven't configured auto-insert-choose yet, do this:
;;
;; M-x customize-apropos RET auto-insert-alist
;;
;; Add these entries:
;;
;; (("\\.\\([Hh]\\|hh\\|hpp\\)\\'" . "C / C++ header")
;;      lambda nil (auto-insert-choose-and-call auto-insert-c-header-alist))
;;
;; (("\\.\\([Cc]\\|cc\\|cpp\\)\\'" . "C / C++ program")
;;      lambda nil (auto-insert-choose-and-call auto-insert-c-body-alist))
;;
;; 
;; If those entries already exist, replace them with the above.

;; See `auto-insert-choose' for more information.

;; Also arrange for this file to be evaluated on start, for instance
;; by putting it into your ".emacs" or linking it into
;; ".emacs.d/site-start.d/" if you use my-site-start (which I highly
;; recommend)

;;;_ , Requires

;; Nothing as such, but it won't do anything meaningful without
;; `auto-insert-choose' 

;;;_. Body
;;;_ , Helper functions
;;;_  . rel-file-name->guard
(defun rel-file-name->guard (rel-file-name)
   ""
   (upcase
      (mapconcat #'identity
	 (split-string rel-file-name "[-\\.\\\\-+,&|^%!]" t)
	 "_")))

;;;_ , Rosegarden
;;;_  . C++ header template
(eval-after-load 'auto-insert-choose
   '(auto-insert-choose-add-entry
       'auto-insert-c-header-alist
       "c++ Rosegarden header"
       (lambda ()
	  (let*
	     (
		(rel-file-name
		   (if (buffer-file-name)
		      (file-name-nondirectory
			 (buffer-file-name))
		      "noname"))
		   
		(base-file-name
		   (file-name-sans-extension rel-file-name))
		(guard-str
		   (rel-file-name->guard 
		      (concat "RG_" rel-file-name)))

		(comment-start (car (split-string comment-start nil t)) )
		(short-desc (read-string "Short description: ")))

		
	     (insert
		"/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-"(substring (current-time-string) -4)" the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#ifndef "guard-str"
#define "guard-str"

"(strings->comment " local includes")"
#include <local/LocalInclude.h>

"(strings->comment " Qt includes")"
#include <QtInclude>

"(strings->comment " STL & other includes")"
#include <vector>

namespace Rosegarden
\{
   "(strings->comment short-desc)"
class "base-file-name"
\{
public:
    "base-file-name"()
    {}

private:
\};
}

#endif "(strings->comment " ifndef "guard-str)"
"

		)))))

;;;_  . C++ body template

(eval-after-load 'auto-insert-choose
   '(auto-insert-choose-add-entry
       'auto-insert-c-body-alist
       "c++ Rosegarden body"
       (lambda ()
	  (let*
	     (
		(rel-file-name
		   (if (buffer-file-name)
		      (file-name-nondirectory
			 (buffer-file-name))
		      "noname"))
		   
		(base-file-name
		   (file-name-sans-extension rel-file-name))

		(comment-start (car (split-string comment-start nil t)) )
		(short-desc (read-string "Short description: ")))

		
	     (insert
		"/* -*- c-basic-offset: 4 indent-tabs-mode: nil -*- vi:set ts=8 sts=4 sw=4: */

/*
    Rosegarden
    A MIDI and audio sequencer and musical notation editor.
    Copyright 2000-"(substring (current-time-string) -4)" the Rosegarden development team.

    Other copyrights also apply to some parts of this work.  Please
    see the AUTHORS file and individual file headers for details.

    This program is free software; you can redistribute it and/or
    modify it under the terms of the GNU General Public License as
    published by the Free Software Foundation; either version 2 of the
    License, or (at your option) any later version.  See the file
    COPYING included with this distribution for more information.
*/

#define RG_MODULE_STRING \"["base-file-name"]\"

#include \""base-file-name".h\"

"(strings->comment " local includes")"
#include <local/LocalInclude.h>

"(strings->comment " Qt includes")"
#include <QtInclude>

"(strings->comment " STL & other includes")"
#include <vector>

namespace Rosegarden
\{
   "(strings->comment short-desc)"
}

#include \""base-file-name".moc\"

"

		)))))



;;;_. Footers
;;;_ , Provides

(provide 'auto-insert-rosegarden-templates)

;;;_ * Local emacs vars.
;;;_  + Local variables:
;;;_  + End:

;;;_ , End
;;; auto-insert-rosegarden-templates.el ends here
