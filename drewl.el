;; Call drewl from emacs

(defcustom drewl-executable "drewl"
  "The path to the drewl executable")

(defvar -drewl-process nil
  "The drewl process")

(defun drewl-quit ()
  (interactive)
  (if (process-live-p -drewl-process)
      (progn
        (process-send-string -drewl-process "quit")
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

(defun drewl-go (url)
  (interactive "sURL: ")
  (process-send-string (-drewl-ensure) (format "go %s\n" url)))
