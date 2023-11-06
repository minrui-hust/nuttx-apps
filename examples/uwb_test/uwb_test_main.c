/****************************************************************************
 * apps/examples/uwb_test/uwb_test_main.c
 *
 * Licensed to the Apache Software Foundation (ASF) under one or more
 * contributor license agreements.  See the NOTICE file distributed with
 * this work for additional information regarding copyright ownership.  The
 * ASF licenses this file to you under the Apache License, Version 2.0 (the
 * "License"); you may not use this file except in compliance with the
 * License.  You may obtain a copy of the License at
 *
 *   http://www.apache.org/licenses/LICENSE-2.0
 *
 * Unless required by applicable law or agreed to in writing, software
 * distributed under the License is distributed on an "AS IS" BASIS, WITHOUT
 * WARRANTIES OR CONDITIONS OF ANY KIND, either express or implied.  See the
 * License for the specific language governing permissions and limitations
 * under the License.
 *
 ****************************************************************************/

/****************************************************************************
 * Included Files
 ****************************************************************************/

#include <nuttx/config.h>

#include <errno.h>
#include <fcntl.h>
#include <poll.h>
#include <sched.h>
#include <stdbool.h>
#include <stdio.h>
#include <stdlib.h>
#include <string.h>
#include <sys/ioctl.h>
#include <unistd.h>

#include <nuttx/input/buttons.h>

/****************************************************************************
 * Pre-processor Definitions
 ****************************************************************************/

#ifndef CONFIG_INPUT_BUTTONS
#error "CONFIG_INPUT_BUTTONS is not defined in the configuration"
#endif

/****************************************************************************
 * Private Data
 ****************************************************************************/

static bool g_uwb_test_daemon_started;
static char g_uwb_msg_send[10] = "hello";

/****************************************************************************
 * Private Functions
 ****************************************************************************/

/****************************************************************************
 * Name: button_daemon
 ****************************************************************************/

static int uwb_test_daemon(int argc, char *argv[]) {
  struct pollfd fds[1];
  int btn_fd, uwb_fd;
  btn_buttonset_t sample;

  /* Indicate that we are running */

  g_uwb_test_daemon_started = true;
  printf("uwb_test: Running\n");

  /* Open the BUTTON driver */

  printf("uwb_test: Opening %s\n", CONFIG_EXAMPLES_UWB_TEST_BUTTON_DEVPATH);
  btn_fd = open(CONFIG_EXAMPLES_UWB_TEST_BUTTON_DEVPATH, O_RDONLY | O_NONBLOCK);
  if (btn_fd < 0) {
    int errcode = errno;
    printf("uwb_test: Failed to open %s: %d\n",
           CONFIG_EXAMPLES_UWB_TEST_BUTTON_DEVPATH, errcode);
    goto errout_with_btn;
  }

  printf("uwb_test: Opening %s\n", CONFIG_EXAMPLES_UWB_TEST_UWB_DEVPATH);
  uwb_fd = open(CONFIG_EXAMPLES_UWB_TEST_UWB_DEVPATH, O_RDWR);
  if (uwb_fd < 0) {
    int errcode = errno;
    printf("uwb_test: Failed to open %s: %d\n",
           CONFIG_EXAMPLES_UWB_TEST_UWB_DEVPATH, errcode);
    goto errout_with_uwb;
  }

  for (;;) {
    /* Prepare the File Descriptor for poll */
    memset(fds, 0, sizeof(fds));

    fds[0].fd = btn_fd;
    fds[0].events = POLLIN;

    /* poll for button pressed */
    poll(fds, 1, -1);

    read(fds[0].fd, (void *)&sample, sizeof(btn_buttonset_t));

    if (sample != 0) {
      printf("uwb_test: send uwb msg\n");
      write(uwb_fd, g_uwb_msg_send, strlen(g_uwb_msg_send));
    }

  }

errout_with_uwb:
  close(uwb_fd);

errout_with_btn:
  close(btn_fd);

  g_uwb_test_daemon_started = false;

  printf("uwb_test: Terminating\n");
  return EXIT_FAILURE;
}

/****************************************************************************
 * Public Functions
 ****************************************************************************/

/****************************************************************************
 * buttons_main
 ****************************************************************************/

int main(int argc, FAR char *argv[]) {
  int ret;

  printf("uwb_test_main: Starting the daemon\n");
  if (g_uwb_test_daemon_started) {
    printf("uwb_test_main: uwb_test_daemon already running\n");
    return EXIT_SUCCESS;
  }

  ret = task_create("uwb_test_daemon", CONFIG_EXAMPLES_UWB_TEST_PRIORITY,
                    CONFIG_EXAMPLES_UWB_TEST_STACKSIZE, uwb_test_daemon, NULL);
  if (ret < 0) {
    int errcode = errno;
    printf("uwb_test_main: ERROR: Failed to start daemon: %d\n", errcode);
    return EXIT_FAILURE;
  }

  printf("uwb_test_main: daemon started\n");
  return EXIT_SUCCESS;
}
