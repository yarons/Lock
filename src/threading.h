#ifndef THREADING_H
#define THREADING_H

#include <adwaita.h>
#include "window.h"

/* Encrypt */
void thread_encrypt_text(GSimpleAction * self, GVariant * parameter,
                         LockWindow * window);
void thread_encrypt_file(GtkButton * self, LockWindow * window);

/* Decrypt */
void thread_decrypt_text(GSimpleAction * self, GVariant * parameter,
                         LockWindow * window);
void thread_decrypt_file(GtkButton * self, LockWindow * window);

/* Sign */
void thread_sign_text(GSimpleAction * self, GVariant * parameter,
                      LockWindow * window);
void thread_sign_file(GtkButton * self, LockWindow * window);

/* Verify */
void thread_verify_text(GSimpleAction * self, GVariant * parameter,
                        LockWindow * window);
void thread_verify_file(GtkButton * self, LockWindow * window);

#endif                          // THREADING_H
