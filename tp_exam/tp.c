/* GPLv2 (c) Airbus */
#include <debug.h>
#include <info.h>

extern info_t *info;

void user1() {
  debug("=== ENTER USER 1 ===\n");
  // TODO: increment shared counter
  // TODO: infinit loop
}

void user2() {
  debug("=== ENTER USER 2 ===\n");
  // TODO: syscall to display shared counter
  // TODO: infinit loop
}

void userland() {
  debug("== ENTER USERLAND ==\n");

  // TODO: launch task 1
  // TODO: launch task 2

  while (1) {}
}

void tp() {
  // TODO: configure segmentation
  // TODO: configure pagination (identity mapping)
  // TODO: configure interruption
}
