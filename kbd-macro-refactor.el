(fset 'camelize-tags-auto-kmacro
   (lambda (&optional arg) "Keyboard macro." (interactive "p") (kmacro-exec-ring-item (quote (") {(xtoggle-camel	b fwxtrtags-rename-s	y" 0 "%d")) arg)))

(fset 'camelize-header-rtags-auto-kmacro
   (lambda (&optional arg) "Keyboard macro." (interactive "p") (kmacro-exec-ring-item (quote (");(xtoggle-came	b fwxrtags	rename-sym	y" 0 "%d")) arg)))

(fset 'insert-default-header-file
   (lambda (&optional arg) "Keyboard macro." (interactive "p") (kmacro-exec-ring-item (quote ([19 35 100 101 102 105 110 101 32 73 78 5 10 10 24 114 105 49 67108896 19 35 2 2 23] 0 "%d")) arg)))
