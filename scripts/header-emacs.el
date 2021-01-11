
(defun take-list-while-helper (lst lst-build str)
  (let ((lst-head (car lst)))
    (if (or (string= lst-head str) (equal (length lst) 0))
      lst-build
      (take-list-while-helper (cdr lst) (cons lst-head lst-build) str)
      )))

(defun take-list-while (lst str)
  (take-list-while-helper lst '() str))

(defun make-header-str-helper (lst str)
  (if (equal (length lst) 0)
    (if (string= str "") str (concat str "_"))
    (make-header-str-helper
     (cdr lst)
     (if (string= str "")
       (upcase (car lst))
       (concat str "_" (upcase (car lst))))
     )))

(defun make-header-str (lst)
  (make-header-str-helper lst ""))

(defun make-namespace-str-helper (lst str)
  (if (equal (length lst) 0)
    (substring str 0 (- (length str) 1))
    (make-namespace-str-helper
     (cdr lst)
     (concat str (concat "namespace " (car lst) " { " ))
     )))

(defun make-namespace-str (lst)
  (make-namespace-str-helper lst ""))

(defun make-end-namespace-str-helper (lst str)
  (if (equal (length lst) 0)
    str
    (make-end-namespace-str-helper
     (cdr lst)
     (if (string= str "")
       (car lst)
       (concat str (concat "::" (car lst))))
     )))

(defun make-end-namespace-str (lst)
  (make-end-namespace-str-helper lst ""))

(defun make-brace (lst str)
  (if (equal (length lst) 0)
      str
    (make-brace (cdr lst) (concat str "}"))))

(defun get-base-vt-dir (dirlist)
  (if (string= (car dirlist) "src")
      (cdr dirlist)
    (get-base-vt-dir (cdr dirlist))))


 ; Create Header Guards with f12
(global-set-key [f12]
 '(lambda ()
    (interactive)
    (if (buffer-file-name)
        (let*
            ((projectName "INCLUDED_")
             (fNameRaw (upcase (file-name-nondirectory (file-name-sans-extension buffer-file-name))))
             (fName (s-replace "-" "_" (s-replace "." "_" fNameRaw)))
             (fNameExtension (upcase (file-name-nondirectory (file-name-extension buffer-file-name))))
             (dirSplit (f-split (file-name-directory (file-name-sans-extension buffer-file-name))))
             (dirNameUntilSrc (mapcar (lambda (x) (s-replace "-" "_" x)) (take-list-while (reverse dirSplit) "src")))
             (dirNameHeader (make-header-str dirNameUntilSrc))
             (fullHeaderName (concat projectName "" dirNameHeader "" fName "_" fNameExtension))
             (fullNSName (make-namespace-str dirNameUntilSrc))
             (fullNSEndName (make-end-namespace-str dirNameUntilSrc))
             (ifDef (concat "\n#if !defined " fullHeaderName "\n#define " fullHeaderName "\n"))
             (begin (point-marker))
             (fNameClean (s-replace "_" "" fName))
             (fullNSBraces (make-brace dirNameUntilSrc ""))
             (headerComment (concat
"""/*
//@HEADER
//                          """ (file-name-nondirectory buffer-file-name) """
//@HEADER
*/
"""
)
             ))
          (progn
            ;;(message "dir-header=%s dir=%s dir-until%s" dirNameHeader (reverse dirSplit) dirNameUntilSrc)
            ; If less then 5 characters are in the buffer, insert the class definition
            (if (< (- (point-max) (point-min)) 5 )
                (progn
                  (insert "\n" fullNSName "\n\nstruct " (capitalize fNameClean) " {\n\nprivate:\n};\n\n" fullNSBraces " /* end namespace " fullNSEndName " */\n")
                  (goto-char (point-min))
                  (next-line-nomark 3)
                  (setq begin (point-marker))
                  )
              )
            ; Insert the Header Guard
            (goto-char (point-min))
            (insert headerComment)
            (insert ifDef)
            (insert (concat "\n" "#include \"vt/config.h\"" "\n"))
            (goto-char (point-max))
            (insert "\n#endif" " /*" fullHeaderName "*/")
            (goto-char begin)
            (save-buffer)
            ;(message "TEST: %s" (reverse (get-base-vt-dir (reverse (split-string (file-name-directory buffer-file-name) "/")))))
            (let*
              ((vt-dirs (reverse (get-base-vt-dir (reverse (split-string (file-name-directory buffer-file-name) "/")))))
               (vt-base-path (string-join vt-dirs "/")))
              ;(message "XXX: %s" (concat "perl " vt-base-path "/scripts/add-license-perl.pl " buffer-file-name " license-template" ))
              (shell-command (concat "perl " vt-base-path "/scripts/add-license-perl.pl " buffer-file-name " " vt-base-path "/scripts/license-template" ))
              )
            (revert-buffer :ignore-auto :noconfirm :preserve-modes)
            )
          )
      (message (concat "Buffer " (buffer-name) " must have a filename"))
      )
    )
 )
