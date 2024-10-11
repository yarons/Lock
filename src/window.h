#ifndef WINDOW_H
#define WINDOW_H

#include <adwaita.h>
#include "application.h"

#define LOCK_TYPE_WINDOW (lock_window_get_type())

G_DECLARE_FINAL_TYPE(LockWindow, lock_window, LOCK, WINDOW,
                     AdwApplicationWindow);

LockWindow *lock_window_new(LockApplication * app);
void lock_window_open(LockWindow * window, GFile * file);

#endif                          // WINDOW_H
