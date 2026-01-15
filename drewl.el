;; Call drewl from emacs

(defcustom drewl-executable "drewl"
  "The path to the drewl executable")

(defvar -drewl-process nil
  "The drewl process")

(defun drewl-quit ()
  (interactive)
  (if (process-live-p -drewl-process)
      (progn
        (process-send-string -drewl-process "quit\n")
        (kill-process -drewl-process)
        (setq -drewl-process nil))))


(defun -drewl-ensure ()
  (if (not (process-live-p -drewl-process))
      (setq -drewl-process
            (make-process
             :name "drewl"
             :command (list  drewl-executable)
             :connection-type 'pipe))
    -drewl-process))

(defun -drewl-go (url)
  (process-send-string (-drewl-ensure) (format "go %s\n" url)))

(defun drewl-go ()
  (interactive)
  (let ((url (thing-at-point 'url)))
    (let ((url (if (eq 'nil url)
                   (read-from-minibuffer "URL: ")
                 url)))
      (unless (eq 'nil url)
        (-drewl-go url)))))

(defun browse-url-drewl (url)
  (-drewl-go url))
