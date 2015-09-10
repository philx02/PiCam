#pragma once

#include "CommonTypedefs.h"

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

CameraPid openCameraChildProcess(const char *iH264FilePath)
{
  pid_t wCameraChild = 0;
  const char * wExecutableLocation = "/usr/bin/raspivid";
  const char * const wArgs[] = { wExecutableLocation, "-o", iH264FilePath, "-v", "-t", "0", "-i", "pause", "-s", "-vf", "-hf", "-fps", "25", "-n", nullptr };
  auto wResult = posix_spawn(&wCameraChild, wExecutableLocation, nullptr, nullptr, const_cast< char * const * >(wArgs), environ);
  if (wResult != 0)
  {
    std::cerr << "Error starting camera, errno is: " << errno << std::endl;
    exit(1);
  }
  return wCameraChild;
}

int openGpio(const char *iGpioPath)
{
  return open(iGpioPath, O_RDONLY);
}

pollfd openSwitchListener(int iGpioFd)
{
  pollfd wPollFd;
  wPollFd.fd = iGpioFd;
  wPollFd.events = POLLPRI | POLLERR;
  return wPollFd;
}

bool readGpio(pollfd &ioPollFd, int iGpioFd)
{
  auto wResult = poll(&ioPollFd, 1, -1);
  lseek(iGpioFd, 0, SEEK_SET);
  char wByte;
  read(iGpioFd, &wByte, 1);
  return wByte == '1';
}

void notifyCameraChildProcess(int iCameraChild)
{
  kill(iCameraChild, SIGUSR1);
}

void waitForCamera()
{
  //std::this_thread::sleep_for(std::chrono::seconds(2));
}

#else

void sendSms(const std::string &iNumber, const std::string &iUrl)
{
  std::string wCommand = "curl \"https://voip.ms/api/v1/rest.php?api_username=pcayouette@spoluck.ca&api_password=0TH7zRXKINXj7Exz8S0c&method=sendSMS&did=4503141161&dst=";
  wCommand += iNumber;
  wCommand += "&message=Door%20opened%20during%20coverage%20time " + iUrl + "\"";
  system(wCommand.c_str());
}

CameraPid openCameraChildProcess(const char *)
{
  return 0;
}

int openGpio(const char *)
{
  return 0;
}

int openSwitchListener(int)
{
  return 0;
}

template< typename T >
bool readGpio(T, int)
{
  // faking gpio readout with stdin
  std::this_thread::sleep_for(std::chrono::hours(1));
  char wValue;
  std::cin >> wValue;
  return wValue == '1';
}

template< typename T >
void notifyCameraChildProcess(T)
{
}

void waitForCamera()
{
}

#endif