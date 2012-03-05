;;;_ auto-insert-choose.el --- Core of choosable auto-insert

;;;_. Headers
;;;_ , License
;; Copyright (C) 2011-2012  Tom Breton (Tehom)

;; Author: Tom Breton (Tehom) <tehom@panix.com>
;; Keywords: convenience

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

;; Commentary:

;; To install:
;;
;; Arrange for this file to be evaluated on start, for instance by
;; putting it into your ".emacs" or linking it into
;; ".emacs.d/site-start.d/" if you use my-site-start (which I highly
;; recommend)

;; Customize each entry in auto-insert-alist that you want to use this with.
;; 
;; M-x customize-apropos RET auto-insert-alist
;;
;; Let's say, for example, C and C++ headers, and you are storing
;; templates in `auto-insert-c-header-alist'.
;;
;; Find or create an entry that begins with something like 
;; (("\\.\\([Hh]\\|hh\\|hpp\\)\\'" . "C / C++ header")
;;      )

;; Replace it with:
;;
;; (("\\.\\([Hh]\\|hh\\|hpp\\)\\'" . "C / C++ header")
;;      lambda nil (auto-insert-choose-and-call auto-insert-c-header-alist))
;;
;; Adapt the string and `auto-insert-c-header-alist' appropriately to
;; the type of file.  See the `auto-insert-alist' docs for format.

;; You may want to add an entry to auto-insert-c-header-alist that
;; copies the original template so it's still available.

;;
;; To create templates:

;;  Use `auto-insert-choose-add-entry'; see its documentation.

;;;_ , Requires



;;;_. Body

;;;_ , auto-insert-choose-and-call
;;;###autoload
(defun auto-insert-choose-and-call (template-alist)
   "Interactively choose and call a function from TEMPLATE-ALIST.
TEMPLATE-ALIST should be a list whose elements are (STRING FUNCTION).
Intended for use in `auto-insert-alist'"
   
   (let*
      ((name (completing-read "Which type? " template-alist))
	 (cell (assoc name template-alist)))
      (when cell
	 (funcall (cadr cell)))))

;;;_ , auto-insert-choose-add-entry

(defun auto-insert-choose-add-entry (sym name func)
   "Add an `auto-insert-choose' entry to SYM.
SYM is the symbol of a list variable.  If it doesn't exist, it will be
created.
NAME is a string naming the entry.
FUNC is a function taking no arguments that inserts the contents.
It may legally do other setup such as setting the buffer mode."

   (when (not (boundp sym))
      (intern (symbol-name sym))
      (set sym '()))

   (add-to-list 
      sym
      (list name func)
      nil
      #'(lambda (a b)
	   (equal (car a)(car b)))))
;;;_ , Convenience functions

;;;_  . strings->comment
(defun strings->comment (&rest strs)
   "Return a comment-string containing STRS.
STRS should be a list of strings."
   (concat
      (apply #'concat comment-start strs)
       comment-end))


;;;_. Footers
;;;_ , Provides

(provide 'auto-insert-choose)

;;;_ * Local emacs vars.
;;;_  + Local variables:
;;;_  + mode: allout
;;;_  + End:

;;;_ , End
;;; auto-insert-choose.el ends here
