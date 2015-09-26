#pragma once

#include <string>

#ifndef WIN32
#include <signal.h>
#include <spawn.h>
#include <sys/types.h>
#include <sys/wait.h>
#include <signal.h>
#include <sys/stat.h>
#include <fcntl.h>
#include <poll.h>
#include <unistd.h>

#include <thread>
#include <iostream>

extern char **environ;

void sendSms(const std::string &iNumber, const std::string &iUrl)
{
  const char * wExecutableLocation = "/usr/bin/curl";
  std::string wArgument = "https://voip.ms/api/v1/rest.php?api_username=pcayouette@spoluck.ca&api_password=0TH7zRXKINXj7Exz8S0c&method=sendSMS&did=4503141161&dst=";
  wArgument += iNumber;
  wArgument += "&message=Door%20opened%20during%20coverage%20time%20";
  wArgument += iUrl;
  const char * const wArgs[] = { wExecutableLocation, wArgument.c_str(), nullptr };
  posix_spawn_file_actions_t wAction;
  posix_spawn_file_actions_init(&wAction);
  posix_spawn_file_actions_addopen(&wAction, STDOUT_FILENO, "/dev/null", O_RDONLY, 0);
  posix_spawn_file_actions_addopen(&wAction, STDERR_FILENO, "/dev/null", O_RDONLY, 0);
  pid_t wPid;
  posix_spawn(&wPid, wExecutableLocation, &wAction, nullptr, const_cast< char * const * >(wArgs), environ);
  posix_spawn_file_actions_destroy(&wAction);
  int wStatus;
  waitpid(wPid, &wStatus, 0);
}

#else

void sendSms(const std::string &iNumber, const std::string &iUrl)
{
  std::string wCommand = "curl \"https://voip.ms/api/v1/rest.php?api_username=pcayouette@spoluck.ca&api_password=0TH7zRXKINXj7Exz8S0c&method=sendSMS&did=4503141161&dst=";
  wCommand += iNumber;
  wCommand += "&message=Door%20opened%20during%20coverage%20time " + iUrl + "\"";
  system(wCommand.c_str());
}

#endif